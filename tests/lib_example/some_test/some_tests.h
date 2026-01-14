#ifndef SOME_TESTS_H
#define SOME_TESTS_H

#include <QtTest>

class SomeTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() const;
    void cleanupTestCase() const;

    void test1_data() const;
    void test1() const;
};

#endif // SOME_TESTS_H
