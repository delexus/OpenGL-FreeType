

#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <GL/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

class Character;

class OpenGLWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow *parent = 0);
    ~OpenGLWindow();

    virtual void render();
    virtual void initialize();
    void setAnimating(bool animating);

    Character getCharacter(QChar character);
    void renderText(const QChar *text, int length, GLfloat x, GLfloat y, GLfloat scale, QVector3D color);
    void setFontSize(int fontsize);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent *event);

    void exposeEvent(QExposeEvent *event);

private:
    bool m_update_pending;
    bool m_animating;

    QOpenGLContext *m_context;
    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    QHash<QChar, Character> m_typeList;
    FT_Library m_ft;
    FT_Face m_face;
    int m_fontsize = 48;
};

class Character
{
public:
    Character() {}

    Character(QOpenGLTexture *texture, QVector2D size, QVector2D bearing, GLuint advance) {
        this->texture = texture;
        this->size = size;
        this->bearing = bearing;
        this->advance = advance;
    }

    QOpenGLTexture      *texture;  // 字形纹理ID
    QVector2D   size;       // 字形大大小
    QVector2D   bearing;    // 字形基于基线和起点的位置
    GLuint      advance;    // 起点到下一个字形起点的距离
};
