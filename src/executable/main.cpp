#include <QMap>
#include <QTimer>
#include <QDebug>
#include <QApplication>

#include <kivk_lib_qt_general.h>

#include <foo.h>

template <typename... Args>
struct Base
{};

template <typename... Args>
struct Derived : Base<Args...>
{};

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    qDebug() << kivk_lib::convertToSet(QStringList{"1", "2", "3"});

    qDebug() << kivk_lib::is_variadic_base_of_v<Base, Base<>>;
    qDebug() << kivk_lib::is_variadic_base_of_v<Base, Derived<int>>;

    qDebug() << kivk_lib::is_variadic_base_of_v<Base, int>;
    qDebug() << kivk_lib::is_variadic_base_of_v<Base, bool>;

    foo();

    QTimer::singleShot(std::chrono::milliseconds{2000}, &application, &QCoreApplication::quit);

    return application.exec();
}



