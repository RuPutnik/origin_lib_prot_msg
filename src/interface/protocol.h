#ifndef PROTOCOL_H
#define PROTOCOL_H
//std::string
#include <string>
//std::memcpy
#include <cstring>
//std::map
#include <map>
//std::vector
#include <vector>
//std::is_integral, std::is_floating_point
#include <type_traits>
//std::unique_ptr, std::make_unique
#include <memory>
//Для преобразований UTF-8 <-> UTF16
#include <locale>
#include <codecvt>

namespace kivk_lib {

/**
 * @par Введение
 * Protocol - класс-обертка массивов байт, реализующий чтение/запись согласно заданному протоколу. Протокол описывается совокупностью полей с некоторым именем и длиной.
 * \n\n
 * @par Настройки
 * Существует два ключевых свойства, во многом диктующих работу класса:\n
 * - Источник буфера, над которым необходимо производить чтение/запись согласно описанному протоколу.
 * - Порядок байт протокола.
 * @par
 * Источник буфера может быть выбран из следующих вариантов:\n
 * - BUFFER_SOURCE::INTERNAL_BUFFER - внутренний буфер, создаваемый/удаляемый автоматически и размер которого всегда находится в соответствии с текущим набором полей в протоколе. Может использоваться, например, при создании и записи некоторых данных с нуля (например, формирование сообщения). Т.е. можно не создавать буфер под данные вручную, а воспользоваться внутренним. Выставляется по умолчанию.
 * - BUFFER_SOURCE::EXTERNAL_BUFFER - внешний буфер, который полностью контролируется вне данного класса.\n
 * @par
 * Порядок байт может быть выбран из следующих вариантов:\n
 * - P_BYTE_ORDER::P_BIG_ENDIAN - порядок байт, при котором первыми идут старшие байты (сетевой порядок байт). Выставляется по умолчанию.
 * - P_BYTE_ORDER::P_LITTLE_ENDIAN - порядок байт, при котором первыми идут младшие байты.
 * @par
 * В случае, если при чтении/записи порядок байт машины (определяется автоматически) не совпадает с порядком байт, заданным в протоколе, будет произведена автоматическая перестановка байт необходимым образом.
 *\n\n
 * @par Ключевые положения по применению
 * - Записывать и читать можно как положительные, так и отрицательные значения: главное при чтении/записи потенциально знаковых значений передать корректный тип T (без unsigned).
 * - Совершенно не важно, где начинается поле: в начале байта или в середине. Не важна также и длина поля: оно может занимать от 1 до 64 бит включительно. Исключения составляют поля, в которые будут записываться/из которых будут читаться числа с плавающей запятой.
 * В таком случае поле всё еще может располагаться в любом месте, однако длина поля должна строго соответствовать используемому типу данных с плавающей запятой (32 для float и 64 для double). Ограничения на длину действуют также в случае задания протоколу порядка байт
 * P_BYTE_ORDER::P_LITTLE_ENDIAN: такой протокол не может содержать полей, чья длина > 8 бит и при этом не кратна 8 битам (например, 9, 13, 22, 60 и т.д.). Это связано с неопределенностями при определении пределов старших и младших байтов.
 * Подробнее можно прочитать в Protocol::setFieldValue(), Protocol::readFieldValue().
 * - Не важна также и суммарная длина всех полей: она может и не дополняться до целого числа байт. Главное соблюдать ограничения, описанные выше.
 * - Для максимальной производительности не рекомендуется создавать объекты протоколов в течение работы программы: создание и добавление полей являются относительно тяжелыми операциями.
 * Рекомендуется создавать все необходимые объекты протоколов сразу при запуске программы. При этом важно не забыть выставить необходимый порядок байт протокола, если он отличен от установленного по умолчанию. Пример создания протокола см. в Protocol::Protocol().
 * - Для формирования данных (например, формирование сообщения) удобнее всего использовать внутренний буфер: не нужно следить за его размером и очисткой. Достаточно лишь указать в протоколе (в конструкторе или при помощи Protocol::appendField()) все необходимые поля и начать заполнять их значениями.
 * При желании, конечно, можно воспользоваться и внешним буфером.
 * - Если необходимо отступить от существующей структуры протокола (например, совокупность нескольких полей воспринять как один массив, или вместо чтения/записи поля длиной 32 бита воспользоваться лишь 16-ю битами), то можно воспользоваться механизмом "призрачных полей".
 * Это поля, обращение к которым происходит не по заранее известному имени, а по передаваемому индексу первого бита и длине в битах.\n
 * Методы, которые используют принцип "призрачных полей":\n Protocol::setGhostFieldValue(), Protocol::setGhostFieldValueAsArray(), Protocol::readGhostFieldValue(), Protocol::readGhostFieldValueAsArray().
 * - Если некоторое поле ("призрачное" или обычное) можно читать/записывать как некоторый массив, то можно воспользоваться методами, позволяющими работать с полями как с массивами:\n Protocol::setFieldValueAsArray(), Protocol::setGhostFieldValueAsArray(), Protocol::readFieldValueAsArray(), Protocol::readGhostFieldValueAsArray().
 * - Для получения длины протокола в байтах можно воспользоваться методом Protocol::getLength(), поскольку длина внутреннего буфера всегда соотвествует текущему набору полей.
 * - Для формирования бинарного дампа буфера достаточно воспользоваться методом получения текущего (Protocol::getWorkingBuffer()) или конкретного (Protocol::getInternalBuffer(), Protocol::getExternalBuffer()) буфера, а затем при помощи метода определения длины протокола Protocol::getLength() записать в файл данные буфера.
 * - Для формирования текстового представления данных можно воспользоваться методом Protocol::getDataVisualization().
 * - Для формирования псевдо-графического предаствления протокола со структурой полей и значениями всех бит можно воспользоваться методом Protocol::getVisualization().
 * @sa Protocol::setFieldValue()
 * @sa Protocol::readFieldValue()
 * @sa Protocol::setBufferSource()
 * @sa Protocol::setByteOrder()
 */
class Protocol
{
public:
    /**
     * Порядок байт, в соответствии с которыми данные записываются и читаются из рабочего буфера
     * @note Приставка "P_" в названии перечисления и его элементов сформирована от слова "Protocol" и необходима для избежания коллизии имён: компилятор уже имеет #define BYTE_ORDER, #define BIG_ENDIAN, #define LITTLE_ENDIAN
     * @sa Protocol::setByteOrder()
     * @sa Protocol::BUFFER_SOURCE
     */
    enum class P_BYTE_ORDER {
        ///Старшие байты располагаются первыми (сетевой порядок байт)
        P_BIG_ENDIAN,
        ///Старшие байты располагаются последними
        P_LITTLE_ENDIAN
    };

    /**
     * Источник данных для чтения и записи согласно протоколу
     * @sa Protocol::setExternalBuffer()
     */
    enum class BUFFER_SOURCE {
        ///Внутренний буффер
        INTERNAL_BUFFER,
        ///Внешний (задаваемый) буффер
        EXTERNAL_BUFFER
    };

    /**
     * Система счисления
     * @sa Protocol::getDataVisualization()
     */
    enum class BASE {
        ///16-ричная
        HEX,
        ///10-ичная
        DEC,
        ///8-ичня
        OCT,
        ///2-ичная
        BIN
    };

    /**
     * Ассоциируемые типы данных с полями для отображения корректных значений в Protocol::getVisualization()
     * @note Внимание! Данное свойство поля совершенно никак не влияет ни на его расположение в памяти, ни на алгоритмы записи/чтения значений.
     * Данное свойство предназначено исключительно для корректного отображения значений полей в Protocol::getVisualization() при установленном printValues = true.
     * @sa Protocol::getVisualization()
     * @sa Protocol::field
     */
    enum class ASSOCIATED_TYPE {
        ///Знаковое целое
        SIGNED_INTEGER,
        ///Беззнаковое целое
        UNSIGNED_INTEGER,
        ///Значение с плавающей запятой
        FLOATING_POINT
    };

    /**
     * @brief Вспомогательная структура, описывающая то или иное поле протокола. Используется в одном из конструкторов и в методе ProtcolWrapper::appendField.
     * @sa Protocol::Protocol()
     * @sa Protocol::appendField()
     * @sa Protocol::getVisualization()
     * @sa Protocol::ASSOCIATED_TYPE
     */
    struct field {
        ///Имя поля, по которому в дальнейшем можно будет обратиться
        std::string name;
        ///Длина поля в битах
        unsigned int bitCount;
        ///Ассоциируемый тип, который может понадобиться только для корректного отображения значений в Protocol::getVisualization()
        ASSOCIATED_TYPE associatedType = ASSOCIATED_TYPE::UNSIGNED_INTEGER;
    };

    /**
     * @brief Конструктор по умолчанию
     * @param[in] byteOrder Порядок байт данных протокола
     * @param[in] bufferSource Источник данных
     * @param[out] externalBuffer Внешний источник данных. Задается в случае параметра bufferSource == BUFFER_SOURCE::EXTERNAL_BUFFER
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::BUFFER_SOURCE
     */
    Protocol(const P_BYTE_ORDER byteOrder = P_BYTE_ORDER::P_BIG_ENDIAN,
             const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
             unsigned char* const externalBuffer = nullptr);

    /**
     * @brief Конструктор с заданным протоколом
     * @param[in] fields Перечень полей протокола
     * @param[in] byteOrder Порядок байт данных протокола
     * @param[in] bufferSource Источник данных
     * @param[out] externalBuffer Внешний источник данных. Задается в случае параметра bufferSource == BUFFER_SOURCE::EXTERNAL_BUFFER
     * @par Пример использования
     * @code{.cpp}
     * //Данные, прочитанные из сокета
     * unsigned char ccsdsMessage[512];
     * //...
     * readDataFromSocket(ccsdsMessage);
     * //...
     * //Обертка данных протоколом (вопреки данному примеру рекомендуется создавать объекты протоколов на старте программы, т.е. в статической памяти)
     * Protocol ccsdsMessageHeader({
                                          {"version",3},    //Версия
                                          {"src_tp",1},     //Тип источника сообщения
                                          {"flg",1},        //Флаг наличия второго заголовка
                                          {"ln",1},         //Признак длинного сообщения
                                          {"reserve",2},    //Не используется
                                          {"source",8},     //Источник сообщения
                                          {"sqflgs",2},     //Флаг сегментированных данных
                                          {"cnum",14},      //Последовательный счетчик сообщения
                                          {"length",16},    //Длина пакета
                                          {"time_secs",32}, //Кол-во секунд с 06.01.1980
                                          {"time_256",8},   //Кол-во 1/256-ых долей секунд
                                          {"activ",2},      //Режим работы
                                          {"chk",1},        //Флаг наличия контрольной суммы
                                          {"spr",1},        //Тип получателся сообщения
                                          {"type",4},       //Тип сообщения
                                          {"subtype",8},    //Подтип сообщения
                                          {"destination",8},//Получатель сообщения
                                          {"udd",32}        //User-definable-data
                                      }, P_BYTE_ORDER::P_BIG_ENDIAN, BUFFER_SOURCE::EXTERNAL_BUFFER, ccsdsMessage);
     * @endcode
     * @note Названия полей могут включать любые символы Unicode кодировки UTF-16 (в том числе кириллица, пробелы и т.д). В случае, если среди задаваемых полей существуют поля с одинаковыми именами или нулевой длиной, ни одно из полей не будет добавлено, и протокол будет пуст
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::BUFFER_SOURCE
     */
    Protocol(const std::vector<field>& fields,
                    const P_BYTE_ORDER byteOrder = P_BYTE_ORDER::P_BIG_ENDIAN,
                    const BUFFER_SOURCE bufferSource = BUFFER_SOURCE::INTERNAL_BUFFER,
                    unsigned char * const externalBuffer = nullptr);

    //Копирование
    Protocol(const Protocol& wrapper);
    Protocol& operator=(const Protocol& wrapper);

    //Перемещение
    Protocol(Protocol&& wrapper);
    Protocol& operator=(Protocol&& wrapper);

    Protocol operator+(const Protocol& prot) const;

    virtual ~Protocol();

    /**
     * @brief Установить порядок байт данных протокола
     * @param[in] byteOrder Порядок байт данных протокола
     * @sa Protocol::P_BYTE_ORDER
     */
    void setByteOrder(const P_BYTE_ORDER byteOrder);
    /**
     * @brief Получить порядок байт данных протокола
     * @return Порядок байт данных протокола
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::getByteOrder()
     */
    P_BYTE_ORDER getByteOrder() const;

    /**
     * @brief Установить источник данных
     * @param[in] bufferSource Источник данных для чтения/записи
     * @sa Protocol::BUFFER_SOURCE
     * @sa Protocol::setByteOrder()
     */
    void setBufferSource(const BUFFER_SOURCE bufferSource);
    /**
     * @brief Получить источник данных
     * @return Источник данных
     * @sa Protocol::BUFFER_SOURCE
     */
    BUFFER_SOURCE getBufferSource() const noexcept;

    /**
     * @brief Получить указатель на внутренний буфер данных
     * @return Указатель на внутренний буфер данных
     */
    const unsigned char* getInternalBuffer() const noexcept;
    /**
     * @brief Получить длину внутреннего буфера
     * @return Длина внутреннего буфера в байтах
     * @note Можно использовать всегда для определения количества байтов, занимаемых полями протокола внезависимости от того, какой из буферов используется: внутренний или внешний.
     * Это связано с тем, что длина внутреннего буфера всегда соотвествтует текущему набору полей
     */
    unsigned int getLength() const noexcept;

    /**
     * @brief Получить указатель на внешний буфер данных
     * @return Указатель на внешний буфер данных.
     * @sa Protocol::setBufferSource()
     * @sa BUFFER_SOURCE::INTERNAL_BUFFER
     * @sa BUFFER_SOURCE::EXTERNAL_BUFFER
     */
    unsigned char* getExternalBuffer() const noexcept;
    /**
     * @brief Установить указатель на внешний буфер данных
     * @param[out] externalBuffer указатель на внешний буфер данных
     * @sa Protocol::setBufferSource()
     * @sa BUFFER_SOURCE::INTERNAL_BUFFER
     * @sa BUFFER_SOURCE::EXTERNAL_BUFFER
     */
    void setExternalBuffer(unsigned char * const externalBuffer);

    /**
     * @brief Скопировать значение данных из заданного буфера
     * @param[out] bufferToCopy указатель заданный буфер
     * @sa Protocol::setBufferSource()
     * @sa BUFFER_SOURCE::INTERNAL_BUFFER
     * @sa BUFFER_SOURCE::EXTERNAL_BUFFER
     */
    void setInternalBufferValues(const unsigned char * const bufferToCopy);

    /**
     * @brief Получить указатель на рабочий буфер данных
     * @return Указатель на рабочий буфер данных в зависимости от установленного Protocol::BUFFER_SOURCE
     * @note В случае отсутствия полей в протоколе будет возвращен nullptr
     */
    unsigned char* getWorkingBuffer() const;

    /**
     * Alias для map, где ключем является имя поля, а значением является порядковый номер поля (начиная с 0)
     * @sa Protocol::getFields()
     */
    using NameToIndMap = std::map<const std::string, const unsigned int>;
    /**
     * @brief Получить map с перечнем полей в протоколе
     * @return Map, где ключем является имя поля, а значением является порядковый номер поля (начиная с 0)
     * @sa Protocol::NameToIndMap
     */
    NameToIndMap getFields() const;

    /**
     * Сгенерировать всевдо-графическое представление протокола по словам (2 байта на строчку). При этом имеется возможность настройки представления:
     * @param[in] drawHeader Рисовать ли заголовок. Заголовок содержит нумерацию бит от 0 до 15, где бит 0 соотвествует самому младшему биту, а бит 15 соответствует самому старшему биту.
     * Таким образом, заголовок будет отличаться в зависимости от установленного порядка байт протокола. Байты выводятся по порядку расположения в памяти слева направо сверху вниз
     * @param[in] firstLineNum Порядковый номер первого слова. Если передано отрицательное значение, то номера слов отображаться не будут
     * @param[in] horizontalBitMargin Размер отступа в символах слева и справа от значения бита. Фактически задаёт символьную ширину бита
     * @param[in] nameLinesCount Количество строк, выдялемых для имени поля
     * @param[in] printValues Печатать ли значение каждого поля в соответствии с выставленным Protocol::ASSOCIATED_TYPE каждому полю
     * @return Текстовое псевдо-графическое представление протокола
     * @par Примеры использования
     * Пример 1
     * @code{.cpp}
     * Protocol ccsdsMessageHeader({
                                              {"version",3},    //Версия
                                              {"src_tp",1},     //Тип источника сообщения
                                              {"flg",1},        //Флаг наличия второго заголовка
                                              {"ln",1},         //Признак длинного сообщения
                                              {"reserve",2},    //Не используетсч
                                              {"source",8},     //Источник сообщения
                                              {"sqflgs",2},     //Флаг сегментированных данных
                                              {"cnum",14},      //Последовательный счетчик сообщения
                                              {"length",16},    //Длина пакета
                                              {"time_secs",32}, //Кол-во секунд с 06.01.1980
                                              {"time_256",8},   //Кол-во 1/256-ых долей секунд
                                              {"activ",2},      //Режим работы
                                              {"chk",1},        //Флаг наличия контрольной суммы
                                              {"spr",1},        //Тип получателся сообщения
                                              {"type",4},       //Тип сообщения
                                              {"subtype",8},    //Подтип сообщения
                                              {"destination",8},//Получатель сообщения
                                              {"udd",32}        //User-definable-data
                                          });
       printf("%s\n", ccsdsMessageHeader.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |version                |src_tp |flg    |ln     |reserve        |source                                                         |
       |     |                       |       |       |       |               |                                                               |
       |_____|___0___'___0___'___0___|___0___|___0___|___0___|___0___'___0___|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
       | 002 |sqflgs         |cnum                                                                                                           |
       |     |               |                                                                                                               |
       |_____|___0___'___0___|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
       | 003 |length                                                                                                                         |
       |     |                                                                                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
       | 004 |time_secs
       |     |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'
       | 005 |                                                                                                                               |
       |     |                                                                                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
       | 006 |time_256                                                       |activ          |chk    |spr    |type                           |
       |     |                                                               |               |       |       |                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|___0___'___0___|___0___|___0___|___0___'___0___'___0___'___0___|
       | 007 |subtype                                                        |destination                                                    |
       |     |                                                               |                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
       | 008 |udd
       |     |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'
       | 009 |                                                                                                                               |
       |     |                                                                                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * Пример 2. Установление значений полям. Обратите внимание, поля "char_2" и "char_3" имеют идентичное битовое представление, но благодаря
     * различающимся установленным Protocol::ASSOCIATED_TYPE вывод значений отличается
     * @code{.cpp}
     * using Type = Protocol::ASSOCIATED_TYPE;
       Protocol wrapper({
                            {"short_1", 16, Type::SIGNED_INTEGER},
                            {"short_2", 16, Type::SIGNED_INTEGER},
                            {"short_3", 16},
                            {"char_1",  8, Type::SIGNED_INTEGER},
                            {"short_4", 16, Type::SIGNED_INTEGER},
                            {"short_5", 16, Type::SIGNED_INTEGER},
                            {"bits_1",  1, Type::SIGNED_INTEGER},
                            {"bits_2",  2, Type::SIGNED_INTEGER},
                            {"bits_3",  3, Type::SIGNED_INTEGER},
                            {"bits_4",  2, Type::SIGNED_INTEGER},
                            {"char_2",  8},
                            {"char_3",  8, Type::SIGNED_INTEGER},
                            {"float_1", 32, Type::FLOATING_POINT},
                            {"double_1",64, Type::FLOATING_POINT}
                        }, Protocol::P_BYTE_ORDER::P_BIG_ENDIAN);
       wrapper.setFieldValue<short>("short_1", 1);
       wrapper.setFieldValue<short>("short_2", 1000);
       wrapper.setFieldValue<unsigned short>("short_3", 32000);
       wrapper.setFieldValue<short>("short_4", -150);
       wrapper.setFieldValue<short>("short_5", -12345);
       wrapper.setFieldValue<char> ("char_1", 5);
       wrapper.setFieldValue<unsigned char> ("char_2", 255);
       wrapper.setFieldValue<char> ("char_3", -1);
       wrapper.setFieldValue<char> ("bits_1", 1);
       wrapper.setFieldValue<char> ("bits_2", 2);
       wrapper.setFieldValue<char> ("bits_3", 3);
       wrapper.setFieldValue<char> ("bits_4", 1);
       wrapper.setFieldValue<float>("float_1", 12.34567f);
       wrapper.setFieldValue<double>("double_1", -3062.7523411);
       printf("%s\n", wrapper.getVisualization(true, 1, 1, 1, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|15 |14 |13 |12 |11 |10 |09 |08 |07 |06 |05 |04 |03 |02 |01 |00 |
       | 001 |short_1                                                        |
       |     |=1                                                             |
       |_____|_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_1_|
       | 002 |short_2                                                        |
       |     |=1000                                                          |
       |_____|_0_'_0_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'_1_'_1_'_0_'_1_'_0_'_0_'_0_|
       | 003 |short_3                                                        |
       |     |=32000                                                         |
       |_____|_0_'_1_'_1_'_1_'_1_'_1_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_|
       | 004 |char_1                         |short_4
       |     |=5                             |=-150
       |_____|_0_'_0_'_0_'_0_'_0_'_1_'_0_'_1_|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_'
       | 005 |                               |short_5
       |     |                               |=-12345
       |_____|_0_'_1_'_1_'_0_'_1_'_0_'_1_'_0_|_1_'_1_'_0_'_0_'_1_'_1_'_1_'_1_'
       | 006 |                               |bit|bits_2 |bits_3     |bits_4 |
       |     |                               |=1 |=2     |=3         |=1     |
       |_____|_1_'_1_'_0_'_0_'_0_'_1_'_1_'_1_|_1_|_1_'_0_|_0_'_1_'_1_|_0_'_1_|
       | 007 |char_2                         |char_3                         |
       |     |=255                           |=-1                            |
       |_____|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_|
       | 008 |float_1
       |     |=12.345670
       |_____|_1_'_1_'_0_'_1_'_1_'_1_'_0_'_1_'_1_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'
       | 009 |                                                               |
       |     |                                                               |
       |_____|_0_'_1_'_0_'_0_'_0_'_1_'_0_'_1_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_1_|
       | 010 |double_1
       |     |=-3062.752341
       |_____|_1_'_1_'_1_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_0_'_0_'_0_'_1_'_1_'_1_'
       | 011 |
       |     |
       |_____|_1_'_1_'_0_'_1_'_1_'_0_'_1_'_0_'_0_'_0_'_1_'_1_'_0_'_0_'_1_'_0_'
       | 012 |
       |     |
       |_____|_1_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'_1_'_0_'_1_'_1_'_0_'_1_'
       | 013 |                                                               |
       |     |                                                               |
       |_____|_1_'_0_'_1_'_0_'_0_'_1_'_1_'_1_'_1_'_1_'_0_'_0_'_0_'_0_'_0_'_0_|
     * @endcode
     * Пример 3. Обратите внимание на порядок байт, отличный от Примера 2 и на то, как это повлияло на расположение различных значений в памяти
     * @code{.cpp}
     * using Type = Protocol::ASSOCIATED_TYPE;
       Protocol wrapper({
                            {"short_1", 16, Type::SIGNED_INTEGER},
                            {"short_2", 16, Type::SIGNED_INTEGER},
                            {"short_3", 16},
                            {"char_1",  8, Type::SIGNED_INTEGER},
                            {"short_4", 16, Type::SIGNED_INTEGER},
                            {"short_5", 16, Type::SIGNED_INTEGER},
                            {"bits_1",  1, Type::SIGNED_INTEGER},
                            {"bits_2",  2, Type::SIGNED_INTEGER},
                            {"bits_3",  3, Type::SIGNED_INTEGER},
                            {"bits_4",  2, Type::SIGNED_INTEGER},
                            {"char_2",  8},
                            {"char_3",  8, Type::SIGNED_INTEGER},
                            {"float_1", 32, Type::FLOATING_POINT},
                            {"double_1",64, Type::FLOATING_POINT}
                        }, Protocol::P_BYTE_ORDER::P_LITTLE_ENDIAN);
       wrapper.setFieldValue<short>("short_1", 1);
       wrapper.setFieldValue<short>("short_2", 1000);
       wrapper.setFieldValue<unsigned short>("short_3", 32000);
       wrapper.setFieldValue<short>("short_4", -150);
       wrapper.setFieldValue<short>("short_5", -12345);
       wrapper.setFieldValue<char> ("char_1", 5);
       wrapper.setFieldValue<unsigned char> ("char_2", 255);
       wrapper.setFieldValue<char> ("char_3", -1);
       wrapper.setFieldValue<char> ("bits_1", 1);
       wrapper.setFieldValue<char> ("bits_2", 2);
       wrapper.setFieldValue<char> ("bits_3", 3);
       wrapper.setFieldValue<char> ("bits_4", 1);
       wrapper.setFieldValue<float>("float_1", 12.34567f);
       wrapper.setFieldValue<double>("double_1", -3062.7523411);
       printf("%s\n", wrapper.getVisualization(true, 1, 1, 1, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|07 |06 |05 |04 |03 |02 |01 |00 |15 |14 |13 |12 |11 |10 |09 |08 |
       | 001 |short_1                                                        |
       |     |=1                                                             |
       |_____|_0_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_|
       | 002 |short_2                                                        |
       |     |=1000                                                          |
       |_____|_1_'_1_'_1_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_1_|
       | 003 |short_3                                                        |
       |     |=32000                                                         |
       |_____|_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'_1_'_1_'_0_'_1_|
       | 004 |char_1                         |short_4
       |     |=5                             |=-150
       |_____|_0_'_0_'_0_'_0_'_0_'_1_'_0_'_1_|_0_'_1_'_1_'_0_'_1_'_0_'_1_'_0_'
       | 005 |                               |short_5
       |     |                               |=-12345
       |_____|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_|_1_'_1_'_0_'_0_'_0_'_1_'_1_'_1_'
       | 006 |                               |bit|bits_2 |bits_3     |bits_4 |
       |     |                               |=1 |=2     |=3         |=1     |
       |_____|_1_'_1_'_0_'_0_'_1_'_1_'_1_'_1_|_1_|_1_'_0_|_0_'_1_'_1_|_0_'_1_|
       | 007 |char_2                         |char_3                         |
       |     |=255                           |=-1                            |
       |_____|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_|_1_'_1_'_1_'_1_'_1_'_1_'_1_'_1_|
       | 008 |float_1
       |     |=12.345670
       |_____|_1_'_1_'_0_'_1_'_1_'_1_'_0_'_1_'_1_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'
       | 009 |                                                               |
       |     |                                                               |
       |_____|_0_'_1_'_0_'_0_'_0_'_1_'_0_'_1_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_1_|
       | 010 |double_1
       |     |=-3062.752341
       |_____|_1_'_1_'_1_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_0_'_0_'_0_'_1_'_1_'_1_'
       | 011 |
       |     |
       |_____|_1_'_1_'_0_'_1_'_1_'_0_'_1_'_0_'_0_'_0_'_1_'_1_'_0_'_0_'_1_'_0_'
       | 012 |
       |     |
       |_____|_1_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_1_'_1_'_1_'_0_'_1_'_1_'_0_'_1_'
       | 013 |                                                               |
       |     |                                                               |
       |_____|_1_'_0_'_1_'_0_'_0_'_1_'_1_'_1_'_1_'_1_'_0_'_0_'_0_'_0_'_0_'_0_|
     * @endcode
     * @note Комбинируя значения различных параметров, можно объединять выводы этого метода у разных протоколов, которые, например, относятся к различным частям одного сообщения.
     * Таким образом, протокол шапки сообщения может быть выведен с шапкой и с начальным номером слова = 1.
     * Последующие протоколы (например, отвечающие за информационную часть сообщения) могут быть выведены таким образом, чтобы шапки не было, а номер строки продолжался.
     * Таким образом, объединив выводы нескольких протоколов, можно получить единое псевдо-графическое представление целого сообщения, обслуживающегося несколькими протоколами.
     * А механизмы настройки ширины/высоты ячеек и вывода значений, записанных в поля, предоставляет мощнейший инструмент для отладки
     * @sa Protocol::ASSOCIATED_TYPE
     * @sa Protocol::setByteOrder()
     * @sa Protocol::setBufferSource()
     * @sa Protocol::setFieldValue()
     * @sa Protocol::getBinaryVisualization()
     * @sa Protocol::getDataVisualization()
     */
    std::string getBinaryVisualization(bool drawHeader = true, int firstLineNum = 1, unsigned int horizontalBitMargin = 3, unsigned int nameLinesCount = 2, int wordBitSize = 16, bool printValues = false) const;

    /**
     * @brief Получить текстовое представление данных. Байты выводятся по порядку расположения в памяти слева направо сверху вниз
     * @param[in] firstLineNumber Номер, с которого следуюет начать нумеровать строки в формате "<номер строки>: ". Если число отрицательное, то номер строки отображаться не будет
     * @param[in] bytesPerLine Количество отображаемых байт на одной строке
     * @param[in] base Система счисления
     * @param[in] spacesBetweenBytes Ставить ли пробел между байтами
     * @return Текстовое представление данных
     * @par Примеры использования
     * Пример 1
     * @code{.cpp}
     * Protocol wrapper({
                                   {"short_1", 16},
                                   {"short_2", 16},
                                   {"short_3", 16},
                                   {"short_4", 16},
                                   {"short_5", 16}
                               }, Protocol::P_BYTE_ORDER::P_BIG_ENDIAN);
       //Сформируем миссив для записи
       short arrayToWrite[5] = {-542, 12362, -16, 11112, 5555};
       //Запишем значения массива в протокол
       wrapper.setGhostFieldValueAsArray<short, 5>(0, 80, arrayToWrite);
       printf("%s\n", wrapper.getDataVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * 1: fd e2
       2: 30 4a
       3: ff f0
       4: 2b 68
       5: 15 b3
     * @endcode
     * Пример 2 (обратите внимание, протокол записывается как LITTLE_ENDIAN, следовательно при выводе видно, что в буфере протокола байты чисел перевернуты относительно Примера 1)
     * @code{.cpp}
     * Protocol wrapper({
                                   {"short_1", 16},
                                   {"short_2", 16},
                                   {"short_3", 16},
                                   {"short_4", 16},
                                   {"short_5", 16}
                               }, Protocol::P_BYTE_ORDER::P_LITTLE_ENDIAN);
       //Сформируем миссив для записи
       short arrayToWrite[5] = {-542, 12362, -16, 11112, 5555};
       //Запишем значения массива в протокол
       wrapper.setGhostFieldValueAsArray<short, 5>(0, 80, arrayToWrite);
       printf("%s\n", wrapper.getDataVisualization(true, 3, Protocol::BASE::HEX, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * 1: e2 fd 4a
       2: 30 f0 ff
       3: 68 2b b3
       4: 15
     * @endcode
     * Пример 3
     * @code{.cpp}
     * Protocol wrapper({
                                   {"short_1", 16},
                                   {"short_2", 16},
                                   {"short_3", 16},
                                   {"short_4", 16},
                                   {"short_5", 16}
                               }, Protocol::P_BYTE_ORDER::P_BIG_ENDIAN);
       //Сформируем миссив для записи
       short arrayToWrite[5] = {-542, 12362, -16, 11112, 5555};
       //Запишем значения массива в протокол
       wrapper.setGhostFieldValueAsArray<short, 5>(0, 80, arrayToWrite);
       printf("%s\n", wrapper.getDataVisualization(false, 2, Protocol::BASE::BIN, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * 11111101 11100010
       00110000 01001010
       11111111 11110000
       00101011 01101000
       00010101 10110011
     * @endcode
     * @sa Protocol::setGhostFieldValueAsArray()
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::getVisualization()
     */
    std::string getDataVisualization(int firstLineNumber = 1, unsigned int bytesPerLine = 2, BASE base = BASE::HEX, bool spacesBetweenBytes = true) const;

    /**
     * @brief Получить указатель на байт, в котором начинается указанное поле. Важно понимать, что указанное поле не обязательно должно иметь свой первый бит в самом начале байта
     * @param[in] fieldName Имя поля
     * @return Указатель на байт, в котором начинается указанное поле
     */
    unsigned char* getFieldBytePointer(const std::string &fieldName) const;

    bool insertFieldAfter(const std::string& fieldAfter, const field& field);

    /**
     * @brief Добавить в конец поле
     * @param[in] field Описание поля.
     * @return Возвращает true в случае успешного добавления поля в протокол и false в случае ошибки (например, из-за того, что поле с некоторым именем уже есть в протоколе)
     * @par Пример использования
     * @code{.cpp}
     * Protocol wrapper({
                            {"field_1",1},
                            {"field_2",4},
                            {"field_3",5},
                        });
       wrapper.appendField({"field_4", 6});
       printf("%s\n", wrapper.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1|field_2                        |field_3                                |field_4                                        |
       |     |       |                               |                                       |                                               |
       |_____|___0___|___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * @note Внутренний буфер при этом удаляется, аллоцируется в соответствии с новым размером и всем байтам устанавливается значение 0!
     * @sa Protocol::field
     * @sa Protocol::getVisualization()
     */
    bool appendField(const field &field);

    /**
     * @brief Добавить в конец протокол (т.е. все его поля)
     * @param[in] wrapper Протокол, чьи поля необходимо добавить
     * @return Возвращает true в случае успешного добавления полей в протокол и false в случае одной из следующих ошибок:
     * - В протоколе уже есть поле с указанным именем
     * - Указанная длина поля равна 0
     * @par Пример использования
     * @code{.cpp}
     * Protocol wrapper1({
                             {"field_1",1},
                             {"field_2",4},
                             {"field_3",5},
                         });
       Protocol wrapper2({
                             {"field_4",1},
                             {"field_5",4},
                             {"field_6",9},
                         });
       wrapper1.appendProtocol(wrapper2);
       printf("%s\n", wrapper1.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1|field_2                        |field_3                                |field_4|field_5                        |field_6
       |     |       |                               |                                       |       |                               |
       |_____|___0___|___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___'___0___|___0___|___0___'___0___'___0___'___0___|___0___'
       | 002 |                                                               |
       |     |                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * @note
     * - Внутренний буфер при этом удаляется, аллоцируется в соответствии с новым размером и всем байтам устанавливается значение 0
     * - В случае, если в добавляемом протоколе окажется поле с тем же именем, что и в текущем протоколе, ни одно из полей протокола не будет добавлено
     * @sa Protocol::field
     * @sa Protocol::getVisualization()
     */
    bool appendProtocol(const Protocol& wrapper);

    /**
     * @brief Удалить последнее поле протокола
     * @note Внутренний буфер при этом удаляется, аллоцируется в соответствии с новым размером и всем байтам устанавливается значение 0!
     */
    void removeLastField();

    /**
     * @brief Удалить все поля протокола
     * @note Внутренний буфер при этом удаляется и выставляется равным nullptr!
     */
    void removeAllFields();

    /**
     * @brief Очистить все значения полей. Во всех байтах рабочего буфера будут выставлен 0
     * @note
     * - Если протокол пуст (нет ни одного поля), то метод ничего не сделает
     * - Обнуляются байты только рабочего буфера
     * @sa Protocol::getWorkingBuffer()
     * @sa Protocol::setBufferSource()
     */
    void clearAllValues();

    /**
     * @brief Установить полю значение. Значения записываются в рабочий буфер в соответствии с указанным порядком байт протокола
     * @param[in] fieldName Имя поля
     * @param[in] value Значение поля
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @par Правила установления значений
     * - Поддерживаются все целочисленные типы\n
     * - Тип "T" обязан быть арифметическим значением (std::is_arithmetic<T>)\n
     * - Длина поля должна быть <= 64 бит\n
     * - Протокол с установленным P_BYTE_ORDER::P_LITTLE_ENDIAN не может иметь полей, длина которых > 8 и при этом не кратна 8 (в битах)\n
     * - Значение с плавающей запятой не может записываться в поле с длиной, отличной от 32 или 64 (в битах)\n
     * - Если передано число с плавающей запятой, а длина указанного поля в битах равна 32, то переданное значение приводится к float (вне зависимости от фактического типа)\n
     * - Если передано число с плавающей запятой, а длина указанного поля в битах равна 64, то переданное значение приводится к double (вне зависимости от фактического типа)\n
     * - Целочисленные отрицательные значения необходимо записывать в поля, чей размер в битах в точности совпадает с sizeof(T)*8. Это связано с особенностями представления отрицательных чисел в памяти: в случае
     * несовпадения размера типа данных целочисленного значения и размера поля (как в одну, так и в другую сторону) невозможно обратно восстановить то же самое число.
     * - Целочисленные значения, не помещающиеся в поле, обрезаются со стороны старших бит. Например:
     * @code{.cpp}
     * //Записываем в поле длиной 2 байта число 1,500,000 (1.5 млн), занимающее 3 байта и имеющее следующее двоичное представление:
       //BIG_ENDIAN:       0001 0110 | 1110 0011 | 0110 0000
       //LITTLE_ENDIAN:    0110 0000 | 1110 0011 | 0001 0110
       //После обрезки значений получим:
       //BIG_ENDIAN:       (обрезаны) | 1110 0011 | 0110 0000
       //LITTLE_ENDIAN:     0110 0000 | 1110 0011 | (обрезаны)
     * Protocol wrapperBigEndian({
                                     {"field_1",16},
                                 }, Protocol::P_BYTE_ORDER::P_BIG_ENDIAN);
       wrapperBigEndian.setFieldValue("field_1", 1500000); //Тип T в данном случае конечно разрешился как int
       printf("BIG_ENDIAN:\n%s\n", wrapperBigEndian.getVisualization().c_str());

       Protocol wrapperLittleEndian({
                                        {"field_1",16},
                                    }, Protocol::P_BYTE_ORDER::P_LITTLE_ENDIAN);
       wrapperLittleEndian.setFieldValue("field_1", 1500000); //Тип T в данном случае конечно разрешился как int
       printf("LITTLE_ENDIAN:\n%s\n", wrapperLittleEndian.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * BIG_ENDIAN:
       |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1                                                                                                                        |
       |     |                                                                                                                               |
       |_____|___1___'___1___'___1___'___0___'___0___'___0___'___1___'___1___'___0___'___1___'___1___'___0___'___0___'___0___'___0___'___0___|

       LITTLE_ENDIAN:
       |_____|  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |
       | 001 |field_1                                                                                                                        |
       |     |                                                                                                                               |
       |_____|___0___'___1___'___1___'___0___'___0___'___0___'___0___'___0___'___1___'___1___'___1___'___0___'___0___'___0___'___1___'___1___|
     * @endcode
     * \n
     * @par Пример использования
     * @code{.cpp}
     * Protocol wrapper({
                            {"field_1",1},
                            {"field_2",4},
                            {"field_3",5},
                            {"field_4",6},
                            {"field_5",32}
                        });
       wrapper.setFieldValue("field_1", 1);
       wrapper.setFieldValue("field_2", 5);
       wrapper.setFieldValue("field_3", 12);
       wrapper.setFieldValue("field_4", 60);
       wrapper.setFieldValue("field_5", 3.1415f);
       printf("field_1 = %d\n", wrapper.readFieldValue<short>("field_1"));
       printf("field_2 = %d\n", wrapper.readFieldValue<char> ("field_2"));
       printf("field_3 = %d\n", wrapper.readFieldValue<int>  ("field_3"));
       printf("field_4 = %d\n", wrapper.readFieldValue<char> ("field_4"));
       printf("field_5 = %.12f\n", wrapper.readFieldValue<float>("field_5"));
       printf("%s\n", wrapper.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * field_1 = 1
       field_2 = 5
       field_3 = 12
       field_4 = 60
       field_5 = 3.141499996185
       |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1|field_2                        |field_3                                |field_4                                        |
       |     |       |                               |                                       |                                               |
       |_____|___1___|___0___'___1___'___0___'___1___|___0___'___1___'___1___'___0___'___0___|___1___'___1___'___1___'___1___'___0___'___0___|
       | 002 |field_5
       |     |
       |_____|___0___'___1___'___0___'___1___'___0___'___1___'___1___'___0___'___0___'___0___'___0___'___0___'___1___'___1___'___1___'___0___'
       | 003 |                                                                                                                               |
       |     |                                                                                                                               |
       |_____|___0___'___1___'___0___'___0___'___1___'___0___'___0___'___1___'___0___'___1___'___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * @sa Protocol::BUFFER_SOURCE
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::getVisualization()
     */
    template<class T>
    void setFieldValue(const std::string& fieldName, const T& value, std::string* errorString = nullptr)
    {
        //Найдем запрашиваемое поле по имени
        mService_fieldItt = getFieldByNameItt(fieldName);
        if(mService_fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. В протоколе нет поля '%s'!\n", fieldName.c_str());
            return;
        }
        const field_description& field = mService_fieldItt->second;
        //Установим значение
        _setFieldValue(field, value, errorString);
    }

    /**
     * @brief Установить "призрачному" полю значение. Призрачное поле - поле, к которому обращение происходит не по некоторому известному в рамках протокола имени, а по индексу первого бита и длине в битах. Позволяет отступать от существующей структуры протокола при необходимости
     * @param[in] fieldFirstBit Индекс первого бита (начиная с 0)
     * @param[in] fieldBitCount Длина поля в битах
     * @param[in] value Значение, которое необходимо присвоить
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @par Правила установления значений
     * Полностью аналогичны таковым для Protocol::setFieldValue()
     */
    template<class T>
    void setGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T& value, std::string* errorString = nullptr)
    {
        _setFieldValue(field_description(fieldFirstBit, fieldBitCount,"<поле_ghost>"), value, errorString);
    }

    /**
     * @brief Установить полю значение, интерпретировав его как массив. Позволяет вписывать в одно поле значения сразу всего массива
     * @param[in] fieldName Имя поля
     * @param[in] array Массив, значения которого необходимо записать в поле
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @tparam N Количество элементов в массиве
     * @par Правила установления значений
     * Полностью аналогичны таковым для Protocol::setFieldValue() с добавлением следующего правила:
     * - Длина поля в битах не ограничена 64-мя битами и должна поровну делиться на переданное количество элементов. Так, например, при длине поля 16 бит и 4-ёх переданных элементах, на каждое из значений отведётся по 4 бита.
     * Стоит помнить о том, что согласно требованиям Protocol::setFieldValue(), для массивов из чисел с плавающей запятой условия жесче: длина поля в битах должна в точности равняться sizeof(T)*N*8, где T - float или double.
     * @par Пример использования
     * @code{.cpp}
     * unsigned char cltuKey[4] = {'C', 'L', 'T', 'U'};
       Protocol headerWithCLTU({
                                   {"field_1", 8},
                                   {"field_2", 4},
                                   {"field_3", 4},
                                   {"CLTU",    32},
                                   {"field_4", 8},
                                   {"field_5", 8}
                               }, Protocol::P_BYTE_ORDER::P_BIG_ENDIAN);
       headerWithCLTU.setFieldValueAsArray<unsigned char, 4>("CLTU", cltuKey);
       printf("%s\n", headerWithCLTU.getVisualization().c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1                                                        |field_2                        |field_3                        |
       |     |                                                               |                               |                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___|
       | 002 |CLTU
       |     |
       |_____|___0___'___1___'___0___'___0___'___0___'___0___'___1___'___1___'___0___'___1___'___0___'___0___'___1___'___1___'___0___'___0___'
       | 003 |                                                                                                                               |
       |     |                                                                                                                               |
       |_____|___0___'___1___'___0___'___1___'___0___'___1___'___0___'___0___'___0___'___1___'___0___'___1___'___0___'___1___'___0___'___1___|
       | 004 |field_4                                                        |field_5                                                        |
       |     |                                                               |                                                               |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * @sa Protocol::getVisualization()
     */
    template<class T, size_t N>
    void setFieldValueAsArray(const std::string& fieldName, const T array[N], std::string* errorString = nullptr)
    {
        //Найдем запрашиваемое поле по имени
        IndToFieldItt fieldItt = getFieldByNameItt(fieldName);
        if(fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray. В протоколе нет поля '%s'!\n", fieldName.c_str());
            return;
        }
        const field_description& field = fieldItt->second;

        //Проверим, чтобы длина поля в битах поровну делилась на все элементы массива
        if(field.bitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray. Длина поля '%s', равная %d бит, не делится поровну на %d элементов!",
                    fieldName.c_str(), field.bitCount, N);
            return;
        }

        //Длина призрачных полей в битах
        const unsigned char ghostFieldLength = field.bitCount / N;

        //Установим каждый элемент массива, считая его "призрачным полем"
        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = field.firstBitInd + i*ghostFieldLength;
            setGhostFieldValue(firstBitInd, ghostFieldLength, array[i], &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    /**
     * @brief Установить "призрачному полю" значение, интерпретировав его как массив. Позволяет отходить от существующей структуры протокола и записывать значения сразу всего массива
     * @param[in] fieldFirstBit Индекс первого бита (начиная с 0)
     * @param[in] fieldBitCount Длина поля в битах
     * @param[in] array Массив, значения которого необходимо записать в "призрачное" поле
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @tparam N Количество элементов в массиве
     * @par Правила установления значений
     * Являются совокупностью правил, описанных в:\n
     * - Protocol::setFieldValueAsArray()
     * - Protocol::setGhostFieldValue()
     * @par Пример использования
     * @code{.cpp}
     * using Type = Protocol::ASSOCIATED_TYPE;
       Protocol wrapper({
                            {"short_1", 16, Type::SIGNED_INTEGER},
                            {"short_2", 16, Type::SIGNED_INTEGER},
                            {"short_3", 16, Type::SIGNED_INTEGER},
                            {"short_4", 16, Type::SIGNED_INTEGER},
                            {"short_5", 16, Type::SIGNED_INTEGER}
                        });
       //Сформируем миссив для записи
       short arrayToWrite[5] = {-542, 12362, -16, 11112, 5555};
       //Запишем значения массива в протокол
       wrapper.setGhostFieldValueAsArray<short, 5>(0, 80, arrayToWrite);
       //Подготовим массив для чтения из протокола
       short arrayToRead[5];
       //Прочитаем массив из поля
       wrapper.readGhostFieldValueAsArray<short, 5>(0, 80, arrayToRead);
       for(int i = 0; i < 5; ++i) {
           printf("arrayToRead[%d] = %d\n", i, arrayToRead[i]);
       }
       printf("%s\n", wrapper.getVisualization(true, 1, 2, 1, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * arrayToRead[0] = -542
       arrayToRead[1] = 12362
       arrayToRead[2] = -16
       arrayToRead[3] = 11112
       arrayToRead[4] = 5555
       |_____| 15  | 14  | 13  | 12  | 11  | 10  | 09  | 08  | 07  | 06  | 05  | 04  | 03  | 02  | 01  | 00  |
       | 001 |short_1                                                                                        |
       |     |=-542                                                                                          |
       |_____|__1__'__1__'__1__'__1__'__1__'__1__'__0__'__1__'__1__'__1__'__1__'__0__'__0__'__0__'__1__'__0__|
       | 002 |short_2                                                                                        |
       |     |=12362                                                                                         |
       |_____|__0__'__0__'__1__'__1__'__0__'__0__'__0__'__0__'__0__'__1__'__0__'__0__'__1__'__0__'__1__'__0__|
       | 003 |short_3                                                                                        |
       |     |=-16                                                                                           |
       |_____|__1__'__1__'__1__'__1__'__1__'__1__'__1__'__1__'__1__'__1__'__1__'__1__'__0__'__0__'__0__'__0__|
       | 004 |short_4                                                                                        |
       |     |=11112                                                                                         |
       |_____|__0__'__0__'__1__'__0__'__1__'__0__'__1__'__1__'__0__'__1__'__1__'__0__'__1__'__0__'__0__'__0__|
       | 005 |short_5                                                                                        |
       |     |=5555                                                                                          |
       |_____|__0__'__0__'__0__'__1__'__0__'__1__'__0__'__1__'__1__'__0__'__1__'__1__'__0__'__0__'__1__'__1__|

     * @endcode
     * @sa Protocol::readGhostFieldValueAsArray()
     * @sa Protocol::getVisualization()
     * @sa Protocol::ASSOCIATED_TYPE
     */
    template<class T, size_t N>
    void setGhostFieldValueAsArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, const T array[N], std::string* errorString = nullptr)
    {
        //Проверим, чтобы длина поля в битах поровну делилась на все элементы массива
        if(fieldBitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValueAsArray. Длина призрачного поля, равная %d бит, не делится поровну на %d элементов!",
                    fieldBitCount, N);
            return;
        }

        //Длина призрачных полей в битах
        const unsigned char ghostFieldLength = fieldBitCount / N;

        //Установим каждый элемент массива, считая его "призрачным полем"
        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = fieldFirstBit + i*ghostFieldLength;
            setGhostFieldValue(firstBitInd, ghostFieldLength, array[i], &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    /**
     * @brief Прочитать значение поля. Значения читаются из рабочего буфера в соответсвии с указанным порядком байт протокола
     * @param[in] fieldName Имя поля
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @return Возвращает числовое значение прочитанного поля, при этом значение интерпретируется в соответствии с заданным типом T
     * @tparam T Тип переданных данных
     * @par Правила чтения значений
     * - Тип "T" обязан быть арифметическим значением (std::is_arithmetic<T>)\n
     * - Длина поля должна быть <= 64 бит\n
     * - Протокол с установленным P_BYTE_ORDER::P_LITTLE_ENDIAN не может иметь полей, длина которых > 8 и при этом не кратна 8 (в битах)\n
     * - Значение с плавающей запятой не может читаться из поля с длиной, отличной от 32 или 64 (в битах)\n
     * - Если читается число с плавающей запятой, а длина указанного поля в битах равна 32, то считается, что прочитанное значение - float (вне зависимости от фактического типа, в которое оно преобразуется при возврате)\n
     * - Если читается число с плавающей запятой, а длина указанного поля в битах равна 64, то считается, что прочитанное значение - double (вне зависимости от фактического типа, в которое оно преобразуется при возврате)\n
     * - Целочисленные значения, не помещающиеся в переданный тип T, обрезаются со стороны старших бит
     * @par Пример использования
     * @code{.cpp}
     * using Type = Protocol::ASSOCIATED_TYPE;
       Protocol wrapper({
                            {"field_1",8,Type::SIGNED_INTEGER},
                            {"field_2",8,Type::SIGNED_INTEGER},
                            {"field_3",8,Type::SIGNED_INTEGER},
                            {"field_4",8,Type::SIGNED_INTEGER},
                            {"field_5",32,Type::FLOATING_POINT}
                        });

       //Запишем во все поля некоторые значения. Последнее значение - float, располагающееся на 32-х битах
       wrapper.setFieldValue("field_1", 1);
       wrapper.setFieldValue("field_2", 5);
       wrapper.setFieldValue("field_3", 12);
       wrapper.setFieldValue("field_4", 60);
       wrapper.setFieldValue("field_5", 3.1415f);
       //Прочитаем значения всех полей. Можно считывать каждое поле по отдельности при помощи имени, в данном случае воспользуемся итератором по map со всеми полями
       const Protocol::NameToIndMap& fields = wrapper.getFields();
       for(auto itt = fields.cbegin(); itt != fields.cend(); ++itt) {
           if(itt->second < 4) printf("%s = %d\n",    itt->first.c_str(), wrapper.readFieldValue<int>  (itt->first));
           else                printf("%s = %.12f\n", itt->first.c_str(), wrapper.readFieldValue<float>(itt->first));
       }
       printf("%s\n", wrapper.getVisualization(true, 1, 3, 1, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * field_1 = 1
       field_2 = 5
       field_3 = 12
       field_4 = 60
       field_5 = 3.141499996185
       |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |field_1                                                        |field_2                                                        |
       |     |=1                                                             |=5                                                             |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___1___|___0___'___0___'___0___'___0___'___0___'___1___'___0___'___1___|
       | 002 |field_3                                                        |field_4                                                        |
       |     |=12                                                            |=60                                                            |
       |_____|___0___'___0___'___0___'___0___'___1___'___1___'___0___'___0___|___0___'___0___'___1___'___1___'___1___'___1___'___0___'___0___|
       | 003 |field_5
       |     |=3.141500
       |_____|___0___'___1___'___0___'___1___'___0___'___1___'___1___'___0___'___0___'___0___'___0___'___0___'___1___'___1___'___1___'___0___'
       | 004 |                                                                                                                               |
       |     |                                                                                                                               |
       |_____|___0___'___1___'___0___'___0___'___1___'___0___'___0___'___1___'___0___'___1___'___0___'___0___'___0___'___0___'___0___'___0___|
     * @endcode
     * @sa Protocol::BUFFER_SOURCE
     * @sa Protocol::P_BYTE_ORDER
     * @sa Protocol::getFields()
     * @sa Protocol::getVisualization()
     * @sa Protocol::ASSOCIATED_TYPE
     */
    template<class T>
    T readFieldValue(const std::string &fieldName, std::string* errorString = nullptr) const
    {
        //Найдем запрашиваемое поле по имени
        IndToFieldItt m_fieldItt = getFieldByNameItt(fieldName);
        if(m_fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. В протоколе нет поля '%s'!\n", fieldName.c_str());
            return T{};
        }
        const field_description& field = m_fieldItt->second;

        return _readFieldValue<T>(field, errorString);
    }

    /**
     * @brief Прочитать из "призрачного" поля значение. Призрачное поле - поле, к которому обращение происходит не по некоторому известному в рамках протокола имени, а по индексу первого бита и длине в битах. Позволяет отступать от существующей структуры протокола при необходимости
     * @param[in] fieldFirstBit Индекс первого бита (начиная с 0)
     * @param[in] fieldBitCount Длина поля в битах
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @return Возвращает числовое значение прочитанного поля
     * @tparam T Тип переданных данных
     * @par Правила чтения значений
     * Полностью аналогичны таковым для Protocol::readFieldValue()
     */
    template<class T>
    T readGhostFieldValue(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, std::string* errorString = nullptr) const
    {
        return _readFieldValue<T>(field_description(fieldFirstBit, fieldBitCount, "<поле_ghost>"), errorString);
    }

    /**
     * @brief Прочитать из поля значение, интерпретировав его как массив. Позволяет читать из одного поля сразу весь массив
     * @param[in] fieldName Имя поля
     * @param[in] array Массив, который необходимо заполнить
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @tparam N Количество элементов в массиве
     * @par Правила чтения значений
     * Полностью аналогичны таковым для Protocol::readFieldValue() с добавлением следующего правила:
     * - Длина поля в битах не ограничена 64-мя битами и должна поровну делиться на переданное количество элементов. Так, например, при длине поля 16 бит и 4-ёх переданных элементах, на каждое из значений отведётся по 4 бита.
     * Стоит помнить о том, что согласно требованиям Protocol::readFieldValue(), для массивов из чисел с плавающей запятой условия жесче: длина поля в битах должна в точности равняться sizeof(T)*N*8, где T - float или double.
     * @par Пример использования
     * @code{.cpp}
       Protocol wrapper({
                            {"int_array", 128}
                        });
       //Сформируем миссив для записи
       int arrayToWrite[4] = {-6231562, 5555555, 11, -9999999};
       //Запишем значения массива в протокол
       wrapper.setFieldValueAsArray<int, 4>("int_array", arrayToWrite);
       //Подготовим массив для чтения из протокола
       int arrayToRead[4];
       //Прочитаем массив из поля
       wrapper.readFieldValueAsArray<int, 4>("int_array", arrayToRead);
       for(int i = 0; i < 4; ++i) {
           printf("int_array[%d] = %d\n", i, arrayToRead[i]);
       }
       printf("%s\n", wrapper.getVisualization(true, 1, 3, 1).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
       int_array[0] = -6231562
       int_array[1] = 5555555
       int_array[2] = 11
       int_array[3] = -9999999
       |_____|  15   |  14   |  13   |  12   |  11   |  10   |  09   |  08   |  07   |  06   |  05   |  04   |  03   |  02   |  01   |  00   |
       | 001 |int_array
       |_____|___1___'___1___'___1___'___1___'___1___'___1___'___1___'___1___'___1___'___0___'___1___'___0___'___0___'___0___'___0___'___0___'
       | 002 |
       |_____|___1___'___1___'___1___'___0___'___1___'___0___'___0___'___1___'___1___'___1___'___1___'___1___'___0___'___1___'___1___'___0___'
       | 003 |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___1___'___0___'___1___'___0___'___1___'___0___'___0___'
       | 004 |
       |_____|___1___'___1___'___0___'___0___'___0___'___1___'___0___'___1___'___0___'___1___'___1___'___0___'___0___'___0___'___1___'___1___'
       | 005 |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'
       | 006 |
       |_____|___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___0___'___1___'___0___'___1___'___1___'
       | 007 |
       |_____|___1___'___1___'___1___'___1___'___1___'___1___'___1___'___1___'___0___'___1___'___1___'___0___'___0___'___1___'___1___'___1___'
       | 008 |                                                                                                                               |
       |_____|___0___'___1___'___1___'___0___'___1___'___0___'___0___'___1___'___1___'___0___'___0___'___0___'___0___'___0___'___0___'___1___|
     * @endcode
     * @sa Protocol::getVisualization()
     */
    template<class T, size_t N>
    void readFieldValueAsArray(const std::string &fieldName, T array[N], std::string* errorString = nullptr) const
    {
        //Найдем запрашиваемое поле по имени
        IndToFieldItt fieldItt = getFieldByNameItt(fieldName);
        if(fieldItt == m_indToFieldMap.cend()) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. В протоколе нет поля '%s'!\n", fieldName.c_str());
            return;
        }
        const field_description& field = fieldItt->second;

        //Проверим, чтобы длина поля в битах поровну делилась на все элементы массива
        if(field.bitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. Длина поля '%s', равная %d бит, не делится поровну на %d элементов!",
                    fieldName.c_str(), field.bitCount, N);
            return;
        }

        //Длина призрачных полей в битах
        const unsigned char ghostFieldLength = field.bitCount / N;

        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = field.firstBitInd + i*ghostFieldLength;
            array[i] = readGhostFieldValue<T>(firstBitInd, ghostFieldLength, &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

    /**
     * @brief Прочитать из "призрачного поля" значение, интерпретировав его как массив. Позволяет отходить от существующей структуры протокола и читать значения сразу всего массива
     * @param[in] fieldFirstBit Индекс первого бита (начиная с 0)
     * @param[in] fieldBitCount Длина поля в битах
     * @param[in] array Массив, значения которого необходимо заполнить
     * @param[out] errorString Указатель на строку, в которую будет возвращен текст ошибки в случае её возникновения
     * @tparam T Тип переданных данных
     * @tparam N Количество элементов в массиве
     * @par Правила чтения значений
     * Являются совокупностью правил, описанных в:\n
     * - Protocol::readFieldValueAsArray()
     * - Protocol::readGhostFieldValue()
     * @par Пример использования
     * @code{.cpp}
       using Type = Protocol::ASSOCIATED_TYPE;
       Protocol wrapper({
                            {"field_1",16,Type::SIGNED_INTEGER},
                            {"field_2",16,Type::SIGNED_INTEGER},
                            {"field_3",16,Type::SIGNED_INTEGER},
                            {"field_4",16,Type::SIGNED_INTEGER},
                            {"field_5",16,Type::SIGNED_INTEGER}
                        });

       //Запишем во все поля некоторые значения
       wrapper.setFieldValue("field_1", -11222);
       wrapper.setFieldValue("field_2", 64);
       wrapper.setFieldValue("field_3", -6323);
       wrapper.setFieldValue("field_4", 5555);
       wrapper.setFieldValue("field_5", -777);

       //Прочитаем сразу весь массив из "призрачного поля", начинающегося с 0 бита и длиной 80 бит
       //Таким образом, мы равномерно охватили все 5 чисел по 16 бит каждое. 16*5 = 80.
       short array[5];
       wrapper.readGhostFieldValueAsArray<short, 5>(0, 80, array);
       for(int i = 0; i < 5; ++i) {
           printf("field_%d = %d\n", i+1, array[i]);
       }
       printf("%s\n", wrapper.getVisualization(true, 1, 1, 1, true).c_str());
     * @endcode
     * Вывод:
     * @code{.unparsed}
     * field_1 = -11222
       field_2 = 64
       field_3 = -6323
       field_4 = 5555
       field_5 = -777
       |_____|15 |14 |13 |12 |11 |10 |09 |08 |07 |06 |05 |04 |03 |02 |01 |00 |
       | 001 |field_1                                                        |
       |     |=-11222                                                        |
       |_____|_1_'_1_'_0_'_1_'_0_'_1_'_0_'_0_'_0_'_0_'_1_'_0_'_1_'_0_'_1_'_0_|
       | 002 |field_2                                                        |
       |     |=64                                                            |
       |_____|_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_0_'_1_'_0_'_0_'_0_'_0_'_0_'_0_|
       | 003 |field_3                                                        |
       |     |=-6323                                                         |
       |_____|_1_'_1_'_1_'_0_'_0_'_1_'_1_'_1_'_0_'_1_'_0_'_0_'_1_'_1_'_0_'_1_|
       | 004 |field_4                                                        |
       |     |=5555                                                          |
       |_____|_0_'_0_'_0_'_1_'_0_'_1_'_0_'_1_'_1_'_0_'_1_'_1_'_0_'_0_'_1_'_1_|
       | 005 |field_5                                                        |
       |     |=-777                                                          |
       |_____|_1_'_1_'_1_'_1_'_1_'_1_'_0_'_0_'_1_'_1_'_1_'_1_'_0_'_1_'_1_'_1_|
     * @endcode
     * @sa Protocol::setFieldValue()
     * @sa Protocol::getVisualization()
     */
    template<class T, size_t N>
    void readGhostFieldValueAsArray(const unsigned int fieldFirstBit, const unsigned int fieldBitCount, T array[N], std::string* errorString = nullptr)
    {
        //Проверим, чтобы длина поля в битах поровну делилась на все элементы массива
        if(fieldBitCount % N) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValueAsArray. Длина призрачного поля, равная %d бит, не делится поровну на %d элементов!",
                    fieldBitCount, N);
            return;
        }

        //Длина призрачных полей в битах
        const unsigned char ghostFieldLength = fieldBitCount / N;

        //Установим каждый элемент массива, считая его "призрачным полем"
        std::string localerror;
        for(unsigned int i = 0; i < N; ++i) {
            if(localerror.length()) break;

            unsigned int firstBitInd = fieldFirstBit + i*ghostFieldLength;
            array[i] = readGhostFieldValue<T>(firstBitInd, ghostFieldLength, &localerror);
        }
        if(localerror.length())
            *errorString = localerror;
    }

private:

    //Структура, содержащая полное описание того или иного поля
    struct field_description {
        field_description(const unsigned int firstBitInd, const unsigned int bitCount, const std::string &name, const ASSOCIATED_TYPE associatedType = ASSOCIATED_TYPE::SIGNED_INTEGER);
        //Индекс стартового байта (начиная от 0)
        unsigned int firstByteInd;
        //Длина поля в байтах
        unsigned int bytesCount;
        //Кол-во занимаемых байт внутри протокола
        unsigned int touchedBytesCount;
        //Индекс стартового бита (начиная от 0)
        unsigned int firstBitInd;
        //Длина поля в битах
        unsigned int bitCount;
        //Отступ битов слева в пером байте протокола
        unsigned char leftSpacing;
        //Отступ битов справа в последнем байте протокола
        unsigned char rightSpacing;
        //Маска для выделения значения из первого (и м.б. единственного) потокольного байта, в котором оно присутствует
        unsigned char firstMask;
        //Масла для выделения значения из последнего протокольного байта, в котором оно присутствует
        unsigned char lastMask;
        //Имя поля
        std::string name;
        //Ассоциируемый тип
        ASSOCIATED_TYPE associatedType;
    };

    //Установить полю значение
    template<class T>
    void _setFieldValue(const field_description& field, const T& value, std::string* errorString = nullptr)
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::setFieldValue. Переданный тип должен быть арифметическим!");

        //Этап 0. Исключим ошибки
        //     1. Протокол, данный в LITTLE_ENDIAN не может иметь полей, длина которых > 8 и не кратна 8
        if(m_protocolByteOrder == P_BYTE_ORDER::P_LITTLE_ENDIAN && field.bitCount > 8 && field.bitCount%8) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Поле '%s' (длина %d) длиннее 8 бит, при этом длина не кратна 8!", field.name.c_str(), field.bitCount);
            return;
        }
        //     2. Значение с плавающей запятой не может иметь длину, отличную от 4 или 8
        if(std::is_floating_point<T>::value && field.bitCount != 32 && field.bitCount != 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Поле '%s' (длина %d) записывается как нецелое, при этом длина не равна 32 или 64 битам!", field.name.c_str(), field.bitCount);
            return;
        }

        if(m_workingBuffer == nullptr) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Текущий буфер (BUFFER_SOURCE::%s) == nullptr!", m_bufferSource==BUFFER_SOURCE::EXTERNAL_BUFFER?"EXTERNAL_BUFFER":"INTERNAL_BUFFER");
            return;
        }

        if(field.bitCount > 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Поле '%s' имеет длину более 64-ти бит!", field.name.c_str(), field.bitCount);
            return;
        }

        //Этап 1. Подготовим буфер для байт нашего числа с учетом доступных байт в протоколе. Добавим +1 байт в конце
        //для вероятного сдвига вправо на Этапе 2.
        memset(mService_rawBytes, 0, 65);
        if(std::is_integral<T>::value) {
            //Сохраним наше число в массив байт, соответствующий N-байтам, занимаемых полем
            //(если поле занимает 10 бит, значит оно занимает 2 байта. Вопрос с отсечением лишних бит решится на этапе финальной записи наложением масок).
            //Нужно решить вопрос с отсеченем лишних старших байт, которые выходят за упомянутые рамки (N):
            // - Для BIG-ENDIAN машины этот вопрос решится перемещением указателя направо (в сторону младших байт)
            //   с целью скопировать только самые младшие N-байт
            // - Для LITTLE-ENDIAN машины указатель уже и так стоит на самом младшем байте, и memcpy
            //   пойдет последовательно копировать самые младшие N-байт.
            mService_val = static_cast<uint64_t>(value);
            mService_ptrToFirstCopyableMostSignificantByte = (unsigned char*)&mService_val;
            if(getMachineByteOrder() == P_BYTE_ORDER::P_BIG_ENDIAN) {
                mService_ptrToFirstCopyableMostSignificantByte += sizeof(uint64_t) - field.bytesCount;
            }
            //Перебросим полученное val в массив байт valueBytes.
            memcpy(mService_rawBytes, mService_ptrToFirstCopyableMostSignificantByte, field.bytesCount);
            //Поменяем местами байты в массиве, если порядок байт машины не совпадает с таковым у протокола
            if(getMachineByteOrder() != m_protocolByteOrder)
                for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                    std::swap(mService_rawBytes[i], mService_rawBytes[field.bytesCount-1-i]);
        } else if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4) {
                float val = static_cast<float>(value);
                memcpy(mService_rawBytes, &val, 4);
            } else if(field.bytesCount == 8) {
                double val = value;
                memcpy(mService_rawBytes, &val, 8);
            }
        }

        //Массив байт valueBytes подготовлен и содержит итоговые байты нашего числа, которое попадет в протокол.

        //Этап 2. Если число в протоколе располагается очень просто, т.е. первый бит находится в в самом начале байта протокола,
        //а последний бит - в самом конце некоторого байта протокола, то записываем по упрощенной схеме (не нужно никаких масок и сдвигов)
        if(field.leftSpacing == 0 && field.rightSpacing == 0) {
            memcpy(m_workingBuffer + field.firstByteInd, mService_rawBytes, field.bytesCount);
            return;
        }

        //Нет, число располагается в протоколе сложнее. Применяем маски к каждому байту протокола, в котором располагается поле.

        //Этап 3. Может, число надо сдвинуть направо для расположения битов как в протоколе
        mService_finalBytes = mService_rawBytes;
        if(field.rightSpacing) {
            shiftRight(mService_rawBytes, static_cast<int>(field.bytesCount + 1), 8-field.rightSpacing);
            //Если первый байт опустел после сдвига, его не учитываем
            if(unsigned char transferableBitsCount = field.bitCount%8) //Есть биты, которые могли после shift_right переместиться все сразу в след.байт
                if(8-field.rightSpacing >= transferableBitsCount) //Сдвиг был >=, чем кол-во этих бит
                    mService_finalBytes = mService_rawBytes+1; //Значит самый первый байт не имеет ни единого бита нашего числа
        }


        //Этап 4. Применяем маски к каждому байту протокола, в котором располагается поле
        unsigned char mask = 0;
        for(uint32_t i = 0; i < field.touchedBytesCount; ++i) {
            //Для первого байта берем firstMask, для промежуточного берем 0xFF, для последнего - lastMask
            mask = i==0?field.firstMask:i!=field.touchedBytesCount-1?0xFF:field.lastMask;
            //Очистим значение этого байта в протоколе
            m_workingBuffer[field.firstByteInd + i] &= ~mask;
            //Теперь запишем в этом байте число
            m_workingBuffer[field.firstByteInd + i] |= mService_finalBytes[i] & mask;
        }
    }

    //Прочитать значение поля
    template<class T>
    T _readFieldValue(const field_description& field, std::string* errorString = nullptr) const
    {
        static_assert(std::is_arithmetic<T>(), "Protocol::readFieldValue. Переданный тип должен быть арифметическим!");

        //Этап 0. Исключим ошибки
        //     1. Протокол, данный в LITTLE_ENDIAN не может иметь полей, длина которых > 8 и не кратна 8
        if(m_protocolByteOrder == P_BYTE_ORDER::P_LITTLE_ENDIAN && field.bitCount > 8 && field.bitCount%8) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. Поле '%s' (длина %d) длиннее 8 бит, при этом длина не кратна 8!", field.name.c_str(), field.bitCount);
            return T{};
        }
        //     2. Значение с плавающей запятой не может иметь длину, отличную от 4 или 8
        if(std::is_floating_point<T>::value && field.bitCount != 32 && field.bitCount != 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::readFieldValue. Поле '%s' (длина %d) считывается как нецелое, при этом длина не равна 32 или 64 битам!", field.name.c_str(), field.bitCount);
            return T{};
        }

        if(m_workingBuffer == nullptr) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Текущий буфер (BUFFER_SOURCE::%s) == nullptr!", m_bufferSource==BUFFER_SOURCE::EXTERNAL_BUFFER?"EXTERNAL_BUFFER":"INTERNAL_BUFFER");
            return T{};
        }

        if(field.bitCount > 64) {
            if(errorString != nullptr) *errorString = getStringFromFormat("Protocol::setFieldValue. Поле '%s' имеет длину более 64-ти бит!", field.name.c_str(), field.bitCount);
            return T{};
        }

        //Этап 1. Получим сухие байты из протокола, в которых есть запрашивемое поле
        memset(mService_rawBytes, 0, 65);
        memcpy(mService_rawBytes, m_workingBuffer + field.firstByteInd, field.touchedBytesCount);

        //Если число записано "не просто", т.е. первый бит числа находится не в самом начале байта протколе и
        //последний бит числа находится не в самом конце последнего байта, то сделаем некоторые доп. преобразования
        mService_finalBytes = mService_rawBytes;
        if(field.rightSpacing || field.leftSpacing) {
            //В первом и последнем байте применим маски, чтобы оставить только чистое значение
            mService_rawBytes[0] &= field.firstMask;
            if(field.touchedBytesCount > 1)
                mService_rawBytes[field.touchedBytesCount-1] &= field.lastMask;

            //Этап 1.2. Выровним положение бит
            if(field.rightSpacing) {
                shiftRight(mService_rawBytes, static_cast<int>(field.touchedBytesCount), field.rightSpacing);

                //Этап 1.3. Подготавливаем указатель на буфер финального размера читаемого поля и заполняем.
                //        Начало этого буфера в общем случае не совпадает с исходным rawBytes => индекс, откуда
                //        копируем, может быть смещен на единицу т.к. после shiftRight первый байт мог опустеть
                mService_finalBytes = mService_rawBytes + field.touchedBytesCount - field.bytesCount;
            }
        }

        //Этап 2.   Если поле считывается как число с плавающей запятой, то через промежуточное число
        //          вернем результат (промежуточное число чтобы в случае, например, чтения float при T=double не было ошибки)
        if(std::is_floating_point<T>::value) {
            if(field.bytesCount == 4)
                return *reinterpret_cast<float*>(mService_finalBytes);
            else if(field.bytesCount == 8)
                return *reinterpret_cast<double*>(mService_finalBytes);
        }

        //Дошли сюда, значит читам как целое число

        //Этап 3. Если порядок байт протокола и машины не совпадает, то перевернем байты
        if(getMachineByteOrder() != m_protocolByteOrder)
            for(uint32_t i = 0; i < field.bytesCount/2; ++i)
                std::swap(mService_finalBytes[i], mService_finalBytes[field.bytesCount-i-1]);

        //Этап 4. Буфер, содержащий наше число, готов. Осталось интерпретировать значение.
        //Нужно решить вопрос с отсеченем лишних старших байт, которые выходят за рамки N = sizeof(T):
        // - Для BIG-ENDIAN машины этот вопрос решится перемещением указателя направо (в сторону младших байт)
        //   с целью скопировать только самые младшие N-байт
        // - Для LITTLE-ENDIAN машины указатель уже и так стоит на самом младшем байте, и reinterpret_cast
        //   пойдет последовательно интерпретировать самые младшие N-байт.
        if(getMachineByteOrder() == P_BYTE_ORDER::P_BIG_ENDIAN)
            mService_finalBytes += field.bytesCount - sizeof(T);
        return *reinterpret_cast<T*>(mService_finalBytes);
    }

    //Получить map-у с индексами полей
    const std::map<const std::string, const unsigned int>& getNameToIndMap() const;
    //Получить map-у с поляи
    const std::map<const unsigned int, const field_description>& getIndToFieldMap() const;

    using NameToIndItt = std::map<const std::string, const unsigned int>::const_iterator;
    using IndToFieldItt = std::map<const unsigned int, const field_description>::const_iterator;

    //Сдвинуть байтовый массив влево
    static void shiftLeft(unsigned char* buf, int len, int shift);
    //Сдвинуть байтовый массив вправо
    static void shiftRight(unsigned char* buf, int len, int shift);

    //Конвертировать UTF-8 в UTF-16
    static std::wstring utf8ToUtf16(const std::string& string);
    //Конвертировать UTF-16 в UTF-8
    static std::string utf16ToUtf8(const std::wstring& string);

    //Порядок байт протокола
    P_BYTE_ORDER m_protocolByteOrder;

    //Режим работы
    BUFFER_SOURCE m_bufferSource;

    //Lazy-инициализация порядка байт машины
    static const P_BYTE_ORDER& getMachineByteOrder();
    //Lazy-инициализация масок для N-битов справа
    static const std::map<unsigned char, unsigned char>& getRightMasks();
    //Lazy-инициализация масок для N-битов справа
    static const std::map<unsigned char, unsigned char>& getLeftMasks();
    //Lazy-инициализация строковых представлений квартетов битов (для перевода в двоичную)
    static const unsigned char** getHalfByteBinary();

    //Определить порядок байт машины
    static P_BYTE_ORDER determineMachineByteOrder();
    //Переаллоцировать внутренний буффер
    void reallocateInternalBuffer();
    //Обновить внутренний буффер
    void updateInternalBuffer();
    //Получить итератор на поле по имени
    IndToFieldItt getFieldByNameItt(const std::string& name) const;

    //Форматировать std::string как в printf
    template<typename... Args>
    static std::string getStringFromFormat(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if(size_s <= 0) return format;
        std::unique_ptr<char[]> buf(new char[size_s]);
        std::snprintf(buf.get(), static_cast<size_t>(size_s), format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size_s - 1);
    }

    //Внутренний буфер
    unsigned char* m_internalBuffer;
    unsigned int m_internalBufferLength;

    //Внешний буфер
    unsigned char* m_externalBuffer;

    //Рабочий буффер
    unsigned char* m_workingBuffer;

    //Сервисные переменные, вынесенные сюда для того, чтобы при их вероятном использовании в
    //_readFieldValue или _setFieldValue не производить лишних инициализаций
    mutable unsigned char* mService_finalBytes;
    mutable IndToFieldItt mService_fieldItt;
    mutable uint64_t mService_val;
    mutable unsigned char* mService_ptrToFirstCopyableMostSignificantByte;
    mutable unsigned char mService_rawBytes[65];

    //Необходимо иметь две map'ы:
    //1) вспомогательная map, через которую можно найти по имени поля нужный индекс в след. мапе
    //2) map полей, привязанных к обычному последовательному индексу [0;n]
    //   Делать ключем имя поля нельзя, поскольку map нарушит их порядок из-за сортировки

    //Индексы полей в протоколе (имя поля, индекс)
    NameToIndMap m_nameToIndMap;
    using NameToIndPair = std::pair<const std::string, const unsigned int>;

private:
    //Поля в протоколе (индекс, поле)
    using IndToFieldMap = std::map<const unsigned int, const field_description>;
    IndToFieldMap m_indToFieldMap;
    using IndToFieldPair = std::pair<const unsigned int, const field_description>;
};

}

#endif // PROTOCOL_H
