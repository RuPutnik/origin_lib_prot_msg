#include <protocol.h>

// *************************************************
// ******************** Protocol *******************
// *************************************************

using kivk_lib::Protocol;

//Конструктор по умолчанию
Protocol::Protocol(const Protocol::P_BYTE_ORDER byteOrder, const Protocol::BUFFER_SOURCE bufferSource, unsigned char * const externalBuffer)
    : m_protocolByteOrder(byteOrder)
    , m_bufferSource(bufferSource)
    , m_internalBuffer(nullptr)
{
    if(bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER)
        m_workingBuffer = m_internalBuffer;
    else {
        m_externalBuffer = externalBuffer;
        m_workingBuffer = m_externalBuffer;
    }
}

//Конструктор с полями
Protocol::Protocol(const std::vector<field> &fields, const P_BYTE_ORDER byteOrder, const BUFFER_SOURCE bufferSource, unsigned char * const externalBuffer)
    : Protocol(byteOrder, bufferSource, externalBuffer)
{
    bool error = false;
    for(const field& field: fields) {
        if(!appendField(field)) {
            //Возникла ошибка при добавлении поля: есть поля с одинаковыми именами или есть поля с нулевой длиной
            error = true;
            break;
        }
    }
    if(error)
        removeAllFields();
}

//Конструктор копирования
Protocol::Protocol(const Protocol& wrapper)
{
    m_internalBuffer = nullptr;

    //1) Аллоцируем аналогичного размера буффер и скопируем в него данные
    m_internalBufferLength = wrapper.m_internalBufferLength;

    if(m_internalBufferLength) {
        m_internalBuffer = new unsigned char[m_internalBufferLength];
        memcpy(m_internalBuffer, wrapper.m_internalBuffer, m_internalBufferLength);
    }

    //2) Cкопируем всё остальное
    m_indToFieldMap = wrapper.m_indToFieldMap;
    m_nameToIndMap = wrapper.m_nameToIndMap;
    m_protocolByteOrder = wrapper.m_protocolByteOrder;
    m_bufferSource = wrapper.m_bufferSource;
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;
}

//Оператор копирования
Protocol& Protocol::operator=(const Protocol& wrapper)
{
    m_internalBuffer = nullptr;

    //1) Аллоцируем аналогичного размера буффер и скопируем в него данные
    m_internalBufferLength = wrapper.m_internalBufferLength;

    if(m_internalBufferLength) {
        m_internalBuffer = new unsigned char[m_internalBufferLength];
        memcpy(m_internalBuffer, wrapper.m_internalBuffer, m_internalBufferLength);
    }

    //2) Cкопируем всё остальное
    m_indToFieldMap = wrapper.m_indToFieldMap;
    m_nameToIndMap = wrapper.m_nameToIndMap;
    m_protocolByteOrder = wrapper.m_protocolByteOrder;
    m_bufferSource = wrapper.m_bufferSource;
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;

    return *this;
}

//Конструктор перемещения
Protocol::Protocol(Protocol &&wrapper)
{
    m_internalBuffer = nullptr;

    //1) Отнимем внутренний буффер
    m_internalBufferLength = wrapper.m_internalBufferLength;
    m_internalBuffer = wrapper.m_internalBuffer;
    wrapper.m_internalBuffer = nullptr;

    //2) Отнимем всё остальное
    m_indToFieldMap = std::move(wrapper.m_indToFieldMap);
    m_nameToIndMap = std::move(wrapper.m_nameToIndMap);
    m_protocolByteOrder = wrapper.m_protocolByteOrder;
    m_bufferSource = wrapper.m_bufferSource;
    m_workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;
}

//Оператор перемещения
Protocol& Protocol::operator=(Protocol&& wrapper)
{
    m_internalBuffer = nullptr;

    //1) Отнимем внутренний буффер
    m_internalBufferLength = wrapper.m_internalBufferLength;
    m_internalBuffer = wrapper.m_internalBuffer;
    wrapper.m_internalBuffer = nullptr;

    //2) Отнимем всё остальное
    m_indToFieldMap = std::move(wrapper.m_indToFieldMap);
    m_nameToIndMap = std::move(wrapper.m_nameToIndMap);
    m_protocolByteOrder = wrapper.m_protocolByteOrder;
    m_bufferSource = wrapper.m_bufferSource;
    m_workingBuffer = wrapper.m_workingBuffer;

    return *this;
}

//Деструктор
Protocol::~Protocol()
{
    if(m_internalBuffer != nullptr)
        delete[] m_internalBuffer;
    m_internalBuffer = nullptr;
}


//Установть порядок байт протокола
void Protocol::setByteOrder(const Protocol::P_BYTE_ORDER byteOrder)
{m_protocolByteOrder = byteOrder;}

//Получить порядок байт протокола
Protocol::P_BYTE_ORDER Protocol::getByteOrder() const
{return m_protocolByteOrder;}

//Установить режим работы
void Protocol::setBufferSource(const Protocol::BUFFER_SOURCE bufferSource)
{
    m_bufferSource = bufferSource;
    if(bufferSource == Protocol::BUFFER_SOURCE::INTERNAL_BUFFER) {
        m_workingBuffer = m_internalBuffer;
    } else {
        m_workingBuffer = m_externalBuffer;
    }
}

//Получить режим работы
Protocol::BUFFER_SOURCE Protocol::getBufferSource() const noexcept{
    return m_bufferSource;
}

//Плучить указатель на внутренний буфер
const unsigned char* Protocol::getInternalBuffer() const noexcept{
    return m_internalBuffer;
}

unsigned int Protocol::getLength() const noexcept{
    return m_internalBufferLength;
}

//Получить указатель на внешний буфер
unsigned char* Protocol::getExternalBuffer() const noexcept{
    return m_externalBuffer;
}

//Установить внешний буфер
void Protocol::setExternalBuffer(unsigned char * const externalBuffer)
{
    m_externalBuffer = externalBuffer;
    if(m_bufferSource == BUFFER_SOURCE::EXTERNAL_BUFFER) {
        m_workingBuffer = m_externalBuffer;
    }
}

void Protocol::setInternalBufferValues(unsigned char * const bufferToCopy)
{
    memcpy(m_internalBuffer, bufferToCopy, m_internalBufferLength);
}

unsigned char *Protocol::getWorkingBuffer() const
{return m_workingBuffer;}

Protocol::NameToIndMap Protocol::getFields() const
{return m_nameToIndMap;}

//Получить граф. представление текущего буфера в протоколе
std::string Protocol::getVisualization(bool drawHeader, int firstLineNum, unsigned int horizontalBitMargin, unsigned int nameLinesCount, bool printValues) const
{
    if(horizontalBitMargin <= 0) horizontalBitMargin = 1;
    if(nameLinesCount <= 0) nameLinesCount = 1;

    if(m_indToFieldMap.empty()) return "Protocol::getVisualization(). Протокол пуст";

    //1. Сформируем массив битов
    std::vector<bool> bits(m_internalBufferLength*8, 0);

    for(uint32_t i = 0; i < m_internalBufferLength; ++i) {
        char c = m_workingBuffer[i];
        for(int j = 7; j >= 0 && c; --j) {
            if(c & 0x1)
                bits[8*i+j] = 1;
            c >>= 1;
        }
    }

    //2 Сформируем таблицу
    std::wstring result;

    //2.1 Сформируем шапку
    std::wstring lessSignificantHeader; //Шапка младших битов
    std::wstring mostSignificantHeader; //Шапка старших битов
    for(int i = 15; i >= 0; i--) {
        if(i < 8)
            lessSignificantHeader += std::wstring(L"|") + std::wstring(horizontalBitMargin-1, ' ') + (i<10?L"0":L"") + std::to_wstring(i) + std::wstring(horizontalBitMargin, ' ');
        else
            mostSignificantHeader += std::wstring(L"|") + std::wstring(horizontalBitMargin-1, ' ') + (i<10?L"0":L"") + std::to_wstring(i) + std::wstring(horizontalBitMargin, ' ');
    }
    unsigned char bitTextLen = lessSignificantHeader.length()/8;

    if(drawHeader) {
        if(firstLineNum >= 0) result = L"|_____";
        if(m_protocolByteOrder == P_BYTE_ORDER::P_BIG_ENDIAN)
            result += mostSignificantHeader + lessSignificantHeader + L"|\n";
        else
            result += lessSignificantHeader + mostSignificantHeader + L"|\n";
    }
    //2.2 Сформируем саму таблицу
    std::vector<std::wstring> nameLines(nameLinesCount); //Строчки для имени
    std::wstring valuesLine; //Строка для значений
    std::wstring bottomLine; //Нижняя строка со значением бит
    int currBitIndInsideBuffer = 0;
    for(IndToFieldItt itt = m_indToFieldMap.cbegin(); itt != m_indToFieldMap.cend(); ++itt) {
        const field_description& field = itt->second;
        //Сформируем верхнюю строчку с именем
        unsigned int availableNameLength = field.bitCount*bitTextLen - 1; //Последний символ для "|"
        std::wstring name = utf8ToUtf16(field.name);
        std::vector<std::wstring> nameLinesForField(nameLinesCount);
        for(uint32_t i = 0; i < nameLinesCount; ++i) {
            std::wstring& fieldLine = nameLinesForField.at(i);
            //Проходим по текущей строчке и смотрим, сколько символов из имени туда надо вписать. Остальное заполним пробелами
            fieldLine = name.substr(0, availableNameLength);
            if(name.length() >= availableNameLength)
                name = name.substr(availableNameLength); //От availableNameLength до конца строки
            else
                name = L"";
            fieldLine = fieldLine + std::wstring(availableNameLength - fieldLine.length(), ' ') + L"|";
            nameLines[i] += fieldLine;
        }

        //Если попросили еще значение вписать:
        if(printValues) {
            std::wstring valueLine;
            //Если тип Floating Point и поле занимает 32 или 64 бита
            if(field.associatedType == ASSOCIATED_TYPE::FLOATING_POINT && (field.bitCount == 32 || field.bitCount == 64)) {
                if(field.bitCount == 32) valueLine = L"="+std::to_wstring(readFieldValue<float> (field.name));
                else if(field.bitCount == 64) valueLine = L"="+std::to_wstring(readFieldValue<double>(field.name));
            }
            //Тогда примем тип за INTEGER. Решим, SIGNED или нет
            //Если запрошен SIGNED, то надо, чтобы кол-во байт было 8,16,32 или 64 (иначе это не имеет смысла)
            else if (field.associatedType == ASSOCIATED_TYPE::SIGNED_INTEGER && (field.bitCount==8||field.bitCount==16||field.bitCount==32||field.bitCount==64)){
                if     (field.bitCount == 8)  valueLine = L"="+std::to_wstring(readFieldValue<int8_t> (field.name));
                else if(field.bitCount == 16) valueLine = L"="+std::to_wstring(readFieldValue<int16_t>(field.name));
                else if(field.bitCount == 32) valueLine = L"="+std::to_wstring(readFieldValue<int32_t>(field.name));
                else if(field.bitCount == 64) valueLine = L"="+std::to_wstring(readFieldValue<int64_t>(field.name));
            } else {
                //Во всех остальных случаях просто UNSIGNED INTEGER
                if     (field.bitCount <= 8)  valueLine = L"="+std::to_wstring(readFieldValue<uint8_t> (field.name));
                else if(field.bitCount <= 16) valueLine = L"="+std::to_wstring(readFieldValue<uint16_t>(field.name));
                else if(field.bitCount <= 32) valueLine = L"="+std::to_wstring(readFieldValue<uint32_t>(field.name));
                else if(field.bitCount <= 64) valueLine = L"="+std::to_wstring(readFieldValue<uint64_t>(field.name));
            }
            //Обрежем, если не влезает в поле
            valueLine = valueLine.substr(0, availableNameLength);
            //Добьем строчку до допустимой длины
            valueLine = valueLine + std::wstring(availableNameLength - valueLine.length(), ' ') + L"|";
            valuesLine += valueLine;
        }

        //Сформируем нижнюю строчку
        std::wstring bottomLineText;
        for(uint32_t j = 0; j < availableNameLength; ++j) {
            if(j >= horizontalBitMargin && ((j - horizontalBitMargin)%bitTextLen)==0){ //середина бита, вставляем его значение
                bottomLineText += std::to_wstring((int)bits[currBitIndInsideBuffer++]);
            }
            else if((j+1) % bitTextLen)
                bottomLineText += L"_";
            else
                bottomLineText += L"'"; //На границах битов поставим кавычку
        }
        bottomLineText += L"|";
        bottomLine += bottomLineText;
    }

    //3. Сейчас наша таблица - одна сплошная строка. Разобьем её на строчки по 16 бит и
    //   добавим номер строки слева
    std::wstring wordNumStr;
    unsigned int currentLineNum = 0;
    while(true) {
        if(nameLines.at(0).length() == 0) break;
        int num = firstLineNum + currentLineNum++;
        wordNumStr = (num<100?std::wstring(L"0"):L"") + (num<10?L"0":L"") + std::to_wstring(num);

        std::wstring lineNumNumberPart = L"| " + wordNumStr + L" |";
        std::wstring lineNumEmptyPart  = L"|     |";
        std::wstring lineNumBottomPart = L"|_____|";

        if(nameLines.at(0).length() >= bitTextLen*16) {
            //Строчки с именем
            for(uint32_t i = 0; i < nameLinesCount; ++i) {
                std::wstring lineWithoutLineNum = nameLines.at(i).substr(0, bitTextLen*16) + L"\n";
                if(i == 0) //Первая строчка
                    result += (firstLineNum>=0?lineNumNumberPart:L"|") + lineWithoutLineNum;
                else
                    result += (firstLineNum>=0?lineNumEmptyPart:L"|")  + lineWithoutLineNum;

                //Обрезаем строчку
                nameLines[i] = nameLines[i].substr(bitTextLen*16);
            }
            //Строчка со значением
            if(printValues) {
                result += (firstLineNum>=0?lineNumEmptyPart:L"|") + valuesLine.substr(0, bitTextLen*16) + L"\n";
                valuesLine = valuesLine.substr(bitTextLen*16);
            }
            //Последняя строчка
            result += (firstLineNum>=0?lineNumBottomPart:L"|") + bottomLine.substr(0, bitTextLen*16) + L"\n";
            //Обрезам строчку
            bottomLine = bottomLine.substr(bitTextLen*16);
        } else {
            //Строчки с именем
            for(uint32_t i = 0; i < nameLinesCount; ++i) {
                if(i == 0) //Первая строчка
                    result += (firstLineNum>=0?lineNumNumberPart:L"|") + nameLines.at(i) + L"\n";
                else
                    result += (firstLineNum>=0?lineNumEmptyPart:L"|")  + nameLines.at(i) + L"\n";
            }
            //Строчка со значением
            if(printValues) {
                result += (firstLineNum>=0?lineNumEmptyPart:L"|") + valuesLine.substr(0, bitTextLen*16) + L"\n";
            }
            //Последняя строчка
            result += (firstLineNum>=0?lineNumBottomPart:L"|") + bottomLine.substr(0, bitTextLen*16) + L"\n";
            break;
        }
    }

    return utf16ToUtf8(result);
}

std::string Protocol::getDataVisualization(int firstLineNumber, unsigned int bytesPerLine, BASE base, bool spacesBetweenBytes) const
{
    if(m_indToFieldMap.empty()) return "Protocol::getDataVisualization(). Протокол пуст";

    if(bytesPerLine == 0) bytesPerLine = 1;

    unsigned int currentBytesOnLine = 0;
    unsigned char* workingBuffer = getWorkingBuffer();

    std::string currentLineText;
    std::string result;
    unsigned int currentLineNumber = 0;

    for(unsigned int i = 0; i < m_internalBufferLength + 1; ++i) {
        //Начало новой линии
        bool itIsFirstByteInLine = false;
        if(currentBytesOnLine == bytesPerLine || i == 0 || i == m_internalBufferLength) {
            itIsFirstByteInLine = true;
            if(currentLineText.length()) {
                result += (i==0?"":"\n") + currentLineText;
            }

            //Это была последняя линия
            if(i == m_internalBufferLength) break;

            //Формируем следующую линию
            currentBytesOnLine = 0;

            currentLineText = (firstLineNumber>=0) ? (std::to_string(firstLineNumber + currentLineNumber++) + ": ") : "";
        }

        //Получим текстовое представление текущего байта
        char byteTextValue[32];
        byteTextValue[0] = 0;

        switch (base) {
        case BASE::HEX:
            sprintf(byteTextValue, "%x", workingBuffer[i]);
            break;
        case BASE::DEC:
            sprintf(byteTextValue, "%d", workingBuffer[i]);
            break;
        case BASE::OCT:
            sprintf(byteTextValue, "%o", workingBuffer[i]);
            break;
        case BASE::BIN://BUG
            sprintf(byteTextValue, "%s%s", getHalfByteBinary()[workingBuffer[i] >> 4], getHalfByteBinary()[workingBuffer[i] & 0x0F]);
            break;
        default:
            break;
        }

        currentLineText += (spacesBetweenBytes?(itIsFirstByteInLine ? "" : " "):"") + std::string(byteTextValue);

        currentBytesOnLine++;
    }

    return result;
}

//Получить указатель на байт, в котором начинается указанное поле
unsigned char* Protocol::getFieldBytePointer(const std::string &fieldName) const
{
    NameToIndItt fieldIndIterator = m_nameToIndMap.find(fieldName);

    if(fieldIndIterator == m_nameToIndMap.cend()) {
        printf("Protocol::getFieldFirstBytePointer. В протоколе нет поля '%s'!\n", fieldName.c_str());
        return nullptr;
    }

    const field_description& field = m_indToFieldMap.find(fieldIndIterator->second)->second;

    //Рабочий буфер
    unsigned char* workingBuffer = m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER ? m_internalBuffer : m_externalBuffer;

    return workingBuffer + field.firstByteInd;
}

bool Protocol::insertFieldAfter(const std::string &fieldAfter, const Protocol::field &field)
{
    /*
    //Проверим, есть ли уже поле с таким же именем
    if(m_nameToIndMap.find(field.name) != m_nameToIndMap.cend()) return false;

    //Проверим, вдруг поле нулевой длины
    if(field.bitCount == 0) return false;

    //Проверим, вдруг указанного поля не существует
    if(m_nameToIndMap.find(fieldAfter) == m_nameToIndMap.cend()) return false;

    NameToIndMap newNameToIndMap;
    IndToFieldMap newIndToFieldMap;

    bool foundFieldAfter = false;
    int newIndex = 0;
    int firstBitOffset = 0;
    for(const NameToIndPair& nameToIndPair : m_nameToIndMap) {
        //Запоминаем данные текущего поля
        const std::string& currFieldName = nameToIndPair.first;
        const int& currInd = nameToIndPair.second;
        const field_description& currDescription = m_indToFieldMap[currInd];

        //Вставляем текущее поле в новые map
        newIndToFieldMap.insert(IndToFieldPair(newIndex, field_description(currDescription.firstBitInd + firstBitOffset, currDescription.bitCount, currDescription.name, currDescription.associatedType)));
        newNameToIndMap.insert(NameToIndPair(currFieldName, newIndex++));

        //Проверяем, вдруг мы только что добавили поле, после которого нужно вставить запрашиваемое
        if(!foundFieldAfter) {
            if(currFieldName == fieldAfter) {
                foundFieldAfter = true;

                //Вставим запрашиваемое новое поле
                newIndToFieldMap.insert(IndToFieldPair(newIndex, field_description(currDescription.firstBitInd + currDescription.bitCount, field.bitCount, field.name, field.associatedType)));
                newNameToIndMap.insert(NameToIndPair(field.name, newIndex++));

                //Все последующие поля будут смещены на это значение
                firstBitOffset = field.bitCount;
            }
        }
    }

    //Заменим старые map-ы на новые
    m_indToFieldMap = newIndToFieldMap;
    m_nameToIndMap = newNameToIndMap;

    reallocateInternalBuffer();
*/
    return true;
}

//Добавить поле
bool Protocol::appendField(const field& field)
{
    //Проверим, есть ли уже поле с таким же именем
    if(m_nameToIndMap.find(field.name) != m_nameToIndMap.cend()) return false;

    //Проверим, вдруг поле нулевой длины
    if(field.bitCount == 0) return false;

    if(m_indToFieldMap.empty()) {
        m_indToFieldMap.insert(IndToFieldPair(0, field_description(0, field.bitCount, field.name, field.associatedType)));
        m_nameToIndMap.insert(NameToIndPair(field.name, 0));
    } else {
        const field_description& lastField = m_indToFieldMap.crbegin()->second;
        const unsigned int newIndex = m_indToFieldMap.crbegin()->first + 1;
        m_indToFieldMap.insert(IndToFieldPair(newIndex, field_description(lastField.firstBitInd+lastField.bitCount, field.bitCount, field.name, field.associatedType)));
        m_nameToIndMap.insert(NameToIndPair(field.name, newIndex));
    }

    updateInternalBuffer();

    return true;
}

bool Protocol::appendProtocol(const Protocol &wrapper)
{
    //Проверим, что все поля полученного протокола имеют отличные имена от всех полей нашего протокола
    for(NameToIndItt itt = wrapper.m_nameToIndMap.cbegin(); itt != wrapper.m_nameToIndMap.cend(); ++itt) {
        if(m_nameToIndMap.find(itt->first) != m_nameToIndMap.cend()) return false;
    }

    for(NameToIndItt itt = wrapper.m_nameToIndMap.cbegin(); itt != wrapper.m_nameToIndMap.cend(); ++itt) {
        const field_description& field = wrapper.m_indToFieldMap.at(itt->second);
        appendField(Protocol::field{field.name, field.bitCount});
    }

    return true;
}

//Удалить последнее поле
void Protocol::removeLastField()
{
    if(m_indToFieldMap.empty()) return;

    const unsigned int lastFieldInd = m_indToFieldMap.crbegin()->first;
    const std::string lastFieldName = m_indToFieldMap.crbegin()->second.name;
    m_indToFieldMap.erase(m_indToFieldMap.find(lastFieldInd));
    m_nameToIndMap.erase(m_nameToIndMap.find(lastFieldName));

    reallocateInternalBuffer();
}

//Удалить все поля
void Protocol::removeAllFields()
{m_indToFieldMap.clear();m_nameToIndMap.clear();reallocateInternalBuffer();}

void Protocol::clearAllValues()
{
    if(m_nameToIndMap.empty()) return;

    memset(getWorkingBuffer(), 0, m_internalBufferLength);
}

//Получить ссылку на map полей
const std::map<const unsigned int, const Protocol::field_description>& Protocol::getIndToFieldMap() const
{return m_indToFieldMap;}

//Получить ссылку на map индексов
const std::map<const std::string, const unsigned int>& Protocol::getNameToIndMap() const
{return m_nameToIndMap;}

//Сдвинуть байтовый массив вправо
void Protocol::shiftRight(unsigned char *buf, int len, int shift)
{
    unsigned char tmp = 0x00, tmp2 = 0x00;
    for (int k = 0; k <= len; ++k) {
        if (k == 0) {
            tmp = buf[k];
            buf[k] >>= shift;
        } else {
            tmp2 = buf[k];
            buf[k] >>= shift;
            buf[k] |= ((tmp & getRightMasks().at(shift)) << (8 - shift));
            if (k != len)
                tmp = tmp2;
        }
    }
}

std::wstring Protocol::utf8ToUtf16(const std::string &string)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(string);
}

std::string Protocol::utf16ToUtf8(const std::wstring &string)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(string);
}

const Protocol::P_BYTE_ORDER &Protocol::getMachineByteOrder()
{
    uint16_t word = 0x1;
    uint8_t bytes[2];
    std::memcpy(bytes, &word, sizeof(uint16_t));
    static const P_BYTE_ORDER machineByteOrder = bytes[0]?P_BYTE_ORDER::P_LITTLE_ENDIAN:P_BYTE_ORDER::P_LITTLE_ENDIAN;
    return machineByteOrder;
}

const std::map<unsigned char, unsigned char>& Protocol::getRightMasks()
{
    static const std::map<unsigned char, unsigned char> rightMasks{
        {0,0x00},
        {1,0x01},
        {2,0x03},
        {3,0x07},
        {4,0x0F},
        {5,0x1F},
        {6,0x3F},
        {7,0x7F}
    };
    return rightMasks;
}

const std::map<unsigned char, unsigned char>& Protocol::getLeftMasks()
{
    static const std::map<unsigned char, unsigned char> leftMasks{
        {0,0x00},
        {1,0x80},
        {2,0xC0},
        {3,0xE0},
        {4,0xF0},
        {5,0xF8},
        {6,0xFC},
        {7,0xFE}
    };
    return leftMasks;
}

const unsigned char** Protocol::getHalfByteBinary()
{
    static const unsigned char halfByteBinary[16][5] = {
     "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };
    return (const unsigned char**)(halfByteBinary);
}

//Сдвинуть байтовый массив влево
void Protocol::shiftLeft(unsigned char *buf, int len, int shift)
{
    char tmp = 0x00, tmp2 = 0x00;
    for (int k = len; k >= 0; k--) {
        if (k == len) {
            tmp = buf[k];
            buf[k] <<= shift;
        } else {
            tmp2 = buf[k];
            buf[k] <<= shift;
            buf[k] |= ((tmp & getLeftMasks().at(shift)) >> (8 - shift));
            tmp = tmp2;
        }
    }
}

//Реаллоцировать внутренний буфер
void Protocol::reallocateInternalBuffer()
{
    if(m_internalBuffer != nullptr) {
        delete[] m_internalBuffer;
        m_internalBuffer = nullptr;
        m_internalBufferLength = 0;
    }

    if(m_indToFieldMap.empty()) return;

    const field_description& lastField = m_indToFieldMap.crbegin()->second;

    unsigned int bits = lastField.firstBitInd + lastField.bitCount;
    m_internalBufferLength = bits/8 + ((bits%8)?1:0);
    m_internalBuffer = new unsigned char[m_internalBufferLength];
    memset(m_internalBuffer, 0, m_internalBufferLength);

    if(m_bufferSource == BUFFER_SOURCE::INTERNAL_BUFFER)
        m_workingBuffer = m_internalBuffer;
}

void Protocol::updateInternalBuffer()
{
    unsigned char* oldBuffer = nullptr;
    unsigned int oldBufferLength = m_internalBufferLength;
    if(m_internalBuffer != nullptr)
    {
        oldBuffer = new unsigned char[oldBufferLength];
        memcpy(oldBuffer, m_internalBuffer, m_internalBufferLength);
    }

    reallocateInternalBuffer();

    //Вернем значения
    if(m_internalBuffer != nullptr && oldBuffer != nullptr)
        memcpy(m_internalBuffer, oldBuffer, std::min(oldBufferLength, m_internalBufferLength));
    if(oldBuffer != nullptr)
        delete[] oldBuffer;
}

Protocol::IndToFieldItt Protocol::getFieldByNameItt(const std::string &name) const
{
    NameToIndItt nameToIndIterator = m_nameToIndMap.find(name);
    if(nameToIndIterator == m_nameToIndMap.cend()) return m_indToFieldMap.cend();
    return m_indToFieldMap.find(nameToIndIterator->second);
}

// *************************************************
// ********************* Field *********************
// *************************************************

Protocol::field_description::field_description(const unsigned int firstBitInd, const unsigned int bitCount, const std::string& name, const ASSOCIATED_TYPE associatedType) {
    this->associatedType = associatedType;
    //Имя поля
    this->name = name;
    //Индекс первого бита в рамках протокола
    this->firstBitInd = firstBitInd;
    //Длина поля в битах
    this->bitCount = bitCount;
    //Длина поля в байтах
    bytesCount = bitCount/8 + ((bitCount%8)?1:0);
    //Индекс первого байта в рамках протокола
    firstByteInd = firstBitInd / 8;
    unsigned int lastByteInd = (firstBitInd + bitCount - 1) / 8;
    //Количество байтов протокола, в которых располагается поле
    touchedBytesCount = lastByteInd - firstByteInd + 1;
    //Отступ первого бита от левой границы байта
    leftSpacing = firstBitInd % 8;
    //Отступ последнего бита от правой границы байта
    rightSpacing = (8 - (firstBitInd + bitCount) % 8) % 8;

    firstMask = 0xFF;
    lastMask = 0xFF;

    //Поле располагается внутри одного байта протокола
    if(touchedBytesCount == 1) {
        firstMask = ~(getLeftMasks().at(leftSpacing) | getRightMasks().at(rightSpacing));
        return;
    }

    //Первая маска
    if(leftSpacing)
        firstMask = getRightMasks().at(8 - leftSpacing);

    //Последняя маска
    if(rightSpacing)
        lastMask = getLeftMasks().at(8 - rightSpacing);
}
