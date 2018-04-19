
#include "openglwindow.h"
#include <QCoreApplication>


OpenGLWindow::OpenGLWindow(QWindow *parent)
    : QWindow(parent)
    , m_update_pending(false)
    , m_animating(false)
    , m_context(0)
    , m_program(0)
    , m_vbo(QOpenGLBuffer::VertexBuffer)
{
    setSurfaceType(QWindow::OpenGLSurface);
}

OpenGLWindow::~OpenGLWindow()
{
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_ft);
    QHashIterator<QChar, Character> iterator(m_typeList);
    while (iterator.hasNext()) {
        iterator.next();
        Character character = iterator.value();
        delete character.texture;
    }
    m_typeList.clear();
    delete m_context;
    delete m_program;
}

void OpenGLWindow::initialize()
{
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.vsh");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.fsh");
    m_program->link();
    m_program->setUniformValue("qt_Texture0", 0);

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(sizeof(GLfloat)*4*4);
    m_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);

    quintptr offset = 0;
    m_program->setAttributeBuffer("qt_Vertex", GL_FLOAT, offset, 2, 4*sizeof(GLfloat));
    m_program->enableAttributeArray("qt_Vertex");
    offset += 2 * sizeof(GLfloat);
    m_program->setAttributeBuffer("qt_TexCoord", GL_FLOAT, offset, 2, 4*sizeof(GLfloat));
    m_program->enableAttributeArray("qt_TexCoord");

    m_vao.release();

    int err;
    if (FT_Init_FreeType(&m_ft))
        qDebug() << "ERROR::FREETYPE: Could not init FreeType Library";

    if (err = FT_New_Face(m_ft, "./default.ttf", 0, &m_face))
        qDebug() << "ERROR::FREETYPE: Failed to load font";

    FT_Set_Pixel_Sizes(m_face, 0, m_fontsize);

}

void OpenGLWindow::setFontSize(int fontsize)
{
    m_fontsize = fontsize;
}

Character OpenGLWindow::getCharacter(QChar character)
{
    if (m_typeList.contains(character))
        return m_typeList.value(character);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // 加载字符的字形
    if (FT_Load_Char(m_face, character.unicode(), FT_LOAD_RENDER)) {
        qDebug() << "ERROR::FREETYTPE: Failed to load Glyph";
        return Character();
    }
    QOpenGLTexture *texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setFormat(QOpenGLTexture::R8_UNorm);
    texture->setSize(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows);
    texture->allocateStorage();
    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8,
                    m_face->glyph->bitmap.buffer);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    Character newChar(
        texture,
        QVector2D(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows),
        QVector2D(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top),
        m_face->glyph->advance.x
    );
    m_typeList.insert(character, newChar);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return newChar;
}

void OpenGLWindow::render()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glViewport(0, 0, 800, 600);

    m_program->bind();
    QString text("我的世界从新开始");
    const QChar *qchar = text.data();

    renderText(qchar, text.size(), 100, 200, 1.0f, QVector3D(1.0f, 0.0f, 0.0f));
}

void OpenGLWindow::renderText(const QChar *text, int length, GLfloat x, GLfloat y, GLfloat scale, QVector3D color)
{
    QMatrix4x4 matrix;
    matrix.ortho(QRect(0, 0, 800, 600));
    m_program->setUniformValue("qt_ModelViewProjectionMatrix", matrix);
    m_program->setUniformValue("textColor", color);
    m_vao.bind();
    for (int i=0; i<length; i++) {
        Character ch = getCharacter(text[i]);
        GLfloat w = ch.size.x() * scale;
        GLfloat h = ch.size.y() * scale;

        GLfloat xpos = x + ch.bearing.x() * scale;
        GLfloat ypos = y + (ch.size.y() - ch.bearing.y()) * scale + m_fontsize - h;

        GLfloat vertices[] = {
           xpos,     ypos,       0.0, 0.0,
           xpos + w, ypos,       1.0, 0.0,
           xpos,     ypos + h,   0.0, 1.0,
           xpos + w, ypos + h,   1.0, 1.0
        };
        ch.texture->bind();
        m_vbo.write(0, vertices, sizeof(vertices));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
        x += (ch.advance >> 6) * scale;
    }
    m_vao.release();
    matrix.setToIdentity();
    m_program->setUniformValue("qt_ModelViewProjectionMatrix", matrix);

}

void OpenGLWindow::renderLater()
{
    if (!m_update_pending) {
        m_update_pending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::UpdateRequest:
        m_update_pending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event)
{
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void OpenGLWindow::renderNow()
{
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!m_context) {
        m_context = new QOpenGLContext(this);
        m_context->setFormat(requestedFormat());
        m_context->create();

        needsInitialize = true;
    }

    m_context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();

    }

    render();

    m_context->swapBuffers(this);

    if (m_animating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating)
{
    m_animating = animating;

    if (animating)
        renderLater();
}


