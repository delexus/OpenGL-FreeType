
#include "openglwindow.h"

#include <QtGui/QGuiApplication>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    OpenGLWindow window;
    window.resize(640, 480);
    window.show();

    window.setAnimating(true);

    return app.exec();
}
