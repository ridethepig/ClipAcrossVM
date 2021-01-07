#include "ClipClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClipClient w;
    w.show();
    return a.exec();
}
