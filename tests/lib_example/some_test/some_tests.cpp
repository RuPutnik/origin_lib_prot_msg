#include "../common/common.h"

#include "some_tests.h"

void SomeTests::initTestCase() const
{
    bar();
    qDebug() << "initTestCase";
}

void SomeTests::cleanupTestCase() const
{
    bar();
    qDebug() << "cleanupTestCase";
}

void SomeTests::test1_data() const
{
    QTest::addColumn<int>("id");
    QTest::addColumn<QString>("name");

    QTest::addRow("#1") << 0 << "name";
}

void SomeTests::test1() const
{
    QFETCH(int, id);
    QFETCH(QString, name);

    QCOMPARE(id, 0);
    QCOMPARE(name, "name");
}

QTEST_MAIN(SomeTests)
