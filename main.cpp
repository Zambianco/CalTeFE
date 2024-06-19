#include "mainwindow_caltefe.h"

#include <QApplication>
#include <QLocale>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));
    QApplication a(argc, argv);
    MainWindow_caltefe w;
    w.show();
    return a.exec();
}
