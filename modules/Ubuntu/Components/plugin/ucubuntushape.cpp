/*
 * Copyright 2013-2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Loïc Molinari <loic.molinari@canonical.com>
 */

#include "ucubuntushape.h"
#include "ucubuntushapetexture.h"
#include "ucunits.h"
#include <QtCore/QPointer>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/qsgflatcolormaterial.h>
#include <QtQuick/private/qquickimage_p.h>

class ShapeMaterial : public QSGMaterial
{
public:
    struct Data
    {
        // Flags must be kept in sync with GLSL fragment shader.
        enum { TexturedFlag = (1 << 0), OverlaidFlag = (1 << 1) };
        QSGTexture* shapeTexture;
        QSGTextureProvider* sourceTextureProvider;
        QRgb backgroundColor;
        QRgb secondaryBackgroundColor;
        QRgb overlayColor;
        quint16 atlasTransform[4];
        quint16 overlaySteps[4];
        QSGTexture::Filtering shapeTextureFiltering;
        quint8 flags;
    };

    ShapeMaterial();
    virtual QSGMaterialType* type() const;
    virtual QSGMaterialShader* createShader() const;
    virtual int compare(const QSGMaterial* other) const;
    const Data* constData() const { return &data_; }
    Data* data() { return &data_; }

private:
    // UCUbuntuShape::updatePaintNode() directly writes to data and ShapeShader::updateState()
    // directly reads from it. We don't bother with getters/setters since it's only meant to be used
    // by the UbuntuShape implementation and makes it easier to maintain.
    Data data_;
};

// -- Scene graph shader ---

class ShapeShader : public QSGMaterialShader
{
public:
    ShapeShader();
    virtual char const* const* attributeNames() const;
    virtual void initialize();
    virtual void updateState(
        const RenderState& state, QSGMaterial* newEffect, QSGMaterial* oldEffect);

private:
    QOpenGLFunctions* glFuncs_;
    int matrixId_;
    int opacityId_;
    int atlasTransformId_;
    int backgroundColorId_;
    int secondaryBackgroundColorId_;
    int overlayColorId_;
    int overlayStepsId_;
    int flagsId_;
};

ShapeShader::ShapeShader()
    : QSGMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/shaders/ucubuntushape.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/shaders/ucubuntushape.frag"));
}

char const* const* ShapeShader::attributeNames() const
{
    static char const* const attributes[] = {
        "positionAttrib", "shapeCoordAttrib", "quadCoordAttrib", 0
    };
    return attributes;
}

void ShapeShader::initialize()
{
    QSGMaterialShader::initialize();
    program()->bind();
    program()->setUniformValue("shapeTexture", 0);
    program()->setUniformValue("sourceTexture", 1);
    glFuncs_ = QOpenGLContext::currentContext()->functions();
    matrixId_ = program()->uniformLocation("matrix");
    opacityId_ = program()->uniformLocation("opacity");
    atlasTransformId_ = program()->uniformLocation("atlasTransform");
    backgroundColorId_ = program()->uniformLocation("backgroundColor");
    secondaryBackgroundColorId_ = program()->uniformLocation("secondaryBackgroundColor");
    overlayColorId_ = program()->uniformLocation("overlayColor");
    overlayStepsId_ = program()->uniformLocation("overlaySteps");
    flagsId_ = program()->uniformLocation("flags");
}

void ShapeShader::updateState(const RenderState& state, QSGMaterial* newEffect,
                              QSGMaterial* oldEffect)
{
    Q_UNUSED(oldEffect);

    const float u8toF32 = 1.0f / 255.0f;
    const float u16ToF32 = 1.0f / static_cast<float>(0xffff);
    const ShapeMaterial::Data* data = static_cast<ShapeMaterial*>(newEffect)->constData();
    QRgb c;

    // Bind shape texture.
    QSGTexture* shapeTexture = data->shapeTexture;
    if (shapeTexture) {
        shapeTexture->setFiltering(data->shapeTextureFiltering);
        shapeTexture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        shapeTexture->setVerticalWrapMode(QSGTexture::ClampToEdge);
        shapeTexture->bind();
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Update color uniforms.
    c = data->backgroundColor;
    program()->setUniformValue(backgroundColorId_, QVector4D(
        qRed(c) * u8toF32, qGreen(c) * u8toF32, qBlue(c) * u8toF32, qAlpha(c) * u8toF32));
    c = data->secondaryBackgroundColor;
    program()->setUniformValue(secondaryBackgroundColorId_, QVector4D(
        qRed(c) * u8toF32, qGreen(c) * u8toF32, qBlue(c) * u8toF32, qAlpha(c) * u8toF32));

    if (data->flags & ShapeMaterial::Data::TexturedFlag) {
        // Bind image texture.
        glFuncs_->glActiveTexture(GL_TEXTURE1);
        QSGTextureProvider* provider = data->sourceTextureProvider;
        QSGTexture* texture = provider ? provider->texture() : NULL;
        if (texture) {
            texture->bind();
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glFuncs_->glActiveTexture(GL_TEXTURE0);
        // Update image uniforms.
        program()->setUniformValue(atlasTransformId_, QVector4D(
            data->atlasTransform[0] * u16ToF32, data->atlasTransform[1] * u16ToF32,
            data->atlasTransform[2] * u16ToF32, data->atlasTransform[3] * u16ToF32));
    }

    if (data->flags & ShapeMaterial::Data::OverlaidFlag) {
        // Update overlay uniforms.
        c = data->overlayColor;
        program()->setUniformValue(overlayColorId_, QVector4D(
            qRed(c) * u8toF32, qGreen(c) * u8toF32, qBlue(c) * u8toF32, qAlpha(c) * u8toF32));
        program()->setUniformValue(overlayStepsId_, QVector4D(
            data->overlaySteps[0] * u16ToF32, data->overlaySteps[1] * u16ToF32,
            data->overlaySteps[2] * u16ToF32, data->overlaySteps[3] * u16ToF32));
    }

    program()->setUniformValue(flagsId_, data->flags);

    // Update QtQuick engine uniforms.
    if (state.isMatrixDirty()) {
        program()->setUniformValue(matrixId_, state.combinedMatrix());
    }
    if (state.isOpacityDirty()) {
        program()->setUniformValue(opacityId_, state.opacity());
    }
}

// --- Scene graph material ---

ShapeMaterial::ShapeMaterial()
    : data_()
{
    setFlag(Blending);
}

QSGMaterialType* ShapeMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader* ShapeMaterial::createShader() const
{
    return new ShapeShader;
}

int ShapeMaterial::compare(const QSGMaterial* other) const
{
    const ShapeMaterial::Data* otherData = static_cast<const ShapeMaterial*>(other)->constData();
    return memcmp(&data_, otherData, sizeof(ShapeMaterial::Data));
}

// --- Scene graph node ---

class ShapeNode : public QSGGeometryNode
{
public:
    struct Vertex {
        float position[2];
        float shapeCoordinate[2];
        float quadCoordinate[2];
        float padding[2];  // Ensure a 32 bytes stride.
    };

    ShapeNode(UCUbuntuShape* item);
    ShapeMaterial* material() { return &material_; }
    void setVertices(float width, float height, float radius, const QQuickItem* source,
                     bool stretched, UCUbuntuShape::HAlignment hAlignment,
                     UCUbuntuShape::VAlignment vAlignment, float shapeCoordinate[][2]);

private:
    UCUbuntuShape* item_;
    QSGGeometry geometry_;
    ShapeMaterial material_;
};

static const unsigned short shapeMeshIndices[] __attribute__((aligned(16))) = {
    0, 4, 1, 5, 2, 6, 3, 7,       // Triangles 1 to 6.
    7, 4,                         // Degenerate triangles.
    4, 8, 5, 9, 6, 10, 7, 11,     // Triangles 7 to 12.
    11, 8,                        // Degenerate triangles.
    8, 12, 9, 13, 10, 14, 11, 15  // Triangles 13 to 18
};

#define ARRAY_SIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

static const struct {
    const unsigned short* const indices;
    int indexCount;       // Number of indices.
    int vertexCount;      // Number of vertices.
    int attributeCount;   // Number of attributes.
    int stride;           // Offset in bytes from one vertex to the other.
    int positionCount;    // Number of components per position.
    int positionType;     // OpenGL type of the position components.
    int shapeCoordCount;  // Number of components per shape texture coordinate.
    int shapeCoordType;   // OpenGL type of the shape texture coordinate components.
    int quadCoordCount;   // Number of components per quad texture coordinate.
    int quadCoordType;    // OpenGL type of the quad texture coordinate components.
    int indexType;        // OpenGL type of the indices.
} shapeMesh = {
    shapeMeshIndices, ARRAY_SIZE(shapeMeshIndices),
    16, 3, sizeof(ShapeNode::Vertex), 2, GL_FLOAT, 2, GL_FLOAT, 2, GL_FLOAT, GL_UNSIGNED_SHORT
};

static const QSGGeometry::AttributeSet& getAttributes()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::create(0, shapeMesh.positionCount, shapeMesh.positionType, true),
        QSGGeometry::Attribute::create(1, shapeMesh.shapeCoordCount, shapeMesh.shapeCoordType),
        QSGGeometry::Attribute::create(2, shapeMesh.quadCoordCount, shapeMesh.quadCoordType)
    };
    static QSGGeometry::AttributeSet attributes = {
        shapeMesh.attributeCount, shapeMesh.stride, data
    };
    return attributes;
}

// Gets the size in bytes of an OpenGL type in the range [GL_BYTE, GL_DOUBLE].
static int sizeOfType(GLenum type)
{
    static int sizes[] = {
        sizeof(char), sizeof(unsigned char), sizeof(short), sizeof(unsigned short), sizeof(int),
        sizeof(unsigned int), sizeof(float), 2, 3, 4, sizeof(double)
    };
    Q_ASSERT(type >= 0x1400 && type <= 0x140a);
    return sizes[type - 0x1400];
}

ShapeNode::ShapeNode(UCUbuntuShape* item)
    : QSGGeometryNode()
    , item_(item)
    , geometry_(getAttributes(), shapeMesh.vertexCount, shapeMesh.indexCount, shapeMesh.indexType)
    , material_()
{
    memcpy(geometry_.indexData(), shapeMesh.indices,
           shapeMesh.indexCount * sizeOfType(shapeMesh.indexType));
    geometry_.setDrawingMode(GL_TRIANGLE_STRIP);
    geometry_.setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry_.setVertexDataPattern(QSGGeometry::AlwaysUploadPattern);
    setGeometry(&geometry_);
    setMaterial(&material_);
    setFlag(UsePreprocess, false);
}

void ShapeNode::setVertices(float width, float height, float radius, const QQuickItem* source,
                            bool stretched, UCUbuntuShape::HAlignment hAlignment,
                            UCUbuntuShape::VAlignment vAlignment, float shapeCoordinate[][2])
{
    ShapeNode::Vertex* vertices = reinterpret_cast<ShapeNode::Vertex*>(geometry_.vertexData());
    const QSGTextureProvider* provider = source ? source->textureProvider() : NULL;
    const QSGTexture* texture = provider ? provider->texture() : NULL;
    float topCoordinate;
    float bottomCoordinate;
    float leftCoordinate;
    float rightCoordinate;
    float radiusCoordinateWidth;
    float radiusCoordinateHeight;

    // FIXME(loicm) With a NxM image, a preserve aspect crop fill mode and a width
    //     component size of N (or a height component size of M), changing the the
    //     height (or width) breaks the 1:1 texel/pixel mapping for odd values.
    if (!stretched && texture) {
        // Preserve source image aspect ratio cropping areas exceeding destination rectangle.
        const float factors[3] = { 0.0f, 0.5f, 1.0f };
        const QSize srcSize = texture->textureSize();
        const float srcRatio = static_cast<float>(srcSize.width()) / srcSize.height();
        const float dstRatio = static_cast<float>(width) / height;
        if (dstRatio <= srcRatio) {
            const float inCoordinateSize = dstRatio / srcRatio;
            const float outCoordinateSize = 1.0f - inCoordinateSize;
            topCoordinate = 0.0f;
            bottomCoordinate = 1.0f;
            leftCoordinate = outCoordinateSize * factors[hAlignment];
            rightCoordinate = 1.0f - (outCoordinateSize * (1.0f - factors[hAlignment]));
            radiusCoordinateHeight = radius / height;
            radiusCoordinateWidth = (radius / width) * inCoordinateSize;
        } else {
            const float inCoordinateSize = srcRatio / dstRatio;
            const float outCoordinateSize = 1.0f - inCoordinateSize;
            topCoordinate = outCoordinateSize * factors[vAlignment];
            bottomCoordinate = 1.0f - (outCoordinateSize * (1.0f - factors[vAlignment]));
            leftCoordinate = 0.0f;
            rightCoordinate = 1.0f;
            radiusCoordinateHeight = (radius / height) * inCoordinateSize;
            radiusCoordinateWidth = radius / width;
        }
    } else {
        // Don't preserve source image aspect ratio stretching it in destination rectangle.
        topCoordinate = 0.0f;
        bottomCoordinate = 1.0f;
        leftCoordinate = 0.0f;
        rightCoordinate = 1.0f;
        radiusCoordinateHeight = radius / height;
        radiusCoordinateWidth = radius / width;
    }

    // Set top row of 4 vertices.
    vertices[0].position[0] = 0.0f;
    vertices[0].position[1] = 0.0f;
    vertices[0].shapeCoordinate[0] = shapeCoordinate[0][0];
    vertices[0].shapeCoordinate[1] = shapeCoordinate[0][1];
    vertices[0].quadCoordinate[0] = leftCoordinate;
    vertices[0].quadCoordinate[1] = topCoordinate;
    vertices[1].position[0] = radius;
    vertices[1].position[1] = 0.0f;
    vertices[1].shapeCoordinate[0] = shapeCoordinate[1][0];
    vertices[1].shapeCoordinate[1] = shapeCoordinate[1][1];
    vertices[1].quadCoordinate[0] = leftCoordinate + radiusCoordinateWidth;
    vertices[1].quadCoordinate[1] = topCoordinate;
    vertices[2].position[0] = width - radius;
    vertices[2].position[1] = 0.0f;
    vertices[2].shapeCoordinate[0] = shapeCoordinate[2][0];
    vertices[2].shapeCoordinate[1] = shapeCoordinate[2][1];
    vertices[2].quadCoordinate[0] = rightCoordinate - radiusCoordinateWidth;
    vertices[2].quadCoordinate[1] = topCoordinate;
    vertices[3].position[0] = width;
    vertices[3].position[1] = 0.0f;
    vertices[3].shapeCoordinate[0] = shapeCoordinate[3][0];
    vertices[3].shapeCoordinate[1] = shapeCoordinate[3][1];
    vertices[3].quadCoordinate[0] = rightCoordinate;
    vertices[3].quadCoordinate[1] = topCoordinate;

    // Set middle-top row of 4 vertices.
    vertices[4].position[0] = 0.0f;
    vertices[4].position[1] = radius;
    vertices[4].shapeCoordinate[0] = shapeCoordinate[4][0];
    vertices[4].shapeCoordinate[1] = shapeCoordinate[4][1];
    vertices[4].quadCoordinate[0] = leftCoordinate;
    vertices[4].quadCoordinate[1] = topCoordinate + radiusCoordinateHeight;
    vertices[5].position[0] = radius;
    vertices[5].position[1] = radius;
    vertices[5].shapeCoordinate[0] = shapeCoordinate[5][0];
    vertices[5].shapeCoordinate[1] = shapeCoordinate[5][1];
    vertices[5].quadCoordinate[0] = leftCoordinate + radiusCoordinateWidth;
    vertices[5].quadCoordinate[1] = topCoordinate + radiusCoordinateHeight;
    vertices[6].position[0] = width - radius;
    vertices[6].position[1] = radius;
    vertices[6].shapeCoordinate[0] = shapeCoordinate[6][0];
    vertices[6].shapeCoordinate[1] = shapeCoordinate[6][1];
    vertices[6].quadCoordinate[0] = rightCoordinate - radiusCoordinateWidth;
    vertices[6].quadCoordinate[1] = topCoordinate + radiusCoordinateHeight;
    vertices[7].position[0] = width;
    vertices[7].position[1] = radius;
    vertices[7].shapeCoordinate[0] = shapeCoordinate[7][0];
    vertices[7].shapeCoordinate[1] = shapeCoordinate[7][1];
    vertices[7].quadCoordinate[0] = rightCoordinate;
    vertices[7].quadCoordinate[1] = topCoordinate + radiusCoordinateHeight;

    // Set middle-bottom row of 4 vertices.
    vertices[8].position[0] = 0.0f;
    vertices[8].position[1] = height - radius;
    vertices[8].shapeCoordinate[0] = shapeCoordinate[8][0];
    vertices[8].shapeCoordinate[1] = shapeCoordinate[8][1];
    vertices[8].quadCoordinate[0] = leftCoordinate;
    vertices[8].quadCoordinate[1] = bottomCoordinate - radiusCoordinateHeight;
    vertices[9].position[0] = radius;
    vertices[9].position[1] = height - radius;
    vertices[9].shapeCoordinate[0] = shapeCoordinate[9][0];
    vertices[9].shapeCoordinate[1] = shapeCoordinate[9][1];
    vertices[9].quadCoordinate[0] = leftCoordinate + radiusCoordinateWidth;
    vertices[9].quadCoordinate[1] = bottomCoordinate - radiusCoordinateHeight;
    vertices[10].position[0] = width - radius;
    vertices[10].position[1] = height - radius;
    vertices[10].shapeCoordinate[0] = shapeCoordinate[10][0];
    vertices[10].shapeCoordinate[1] = shapeCoordinate[10][1];
    vertices[10].quadCoordinate[0] = rightCoordinate - radiusCoordinateWidth;
    vertices[10].quadCoordinate[1] = bottomCoordinate - radiusCoordinateHeight;
    vertices[11].position[0] = width;
    vertices[11].position[1] = height - radius;
    vertices[11].shapeCoordinate[0] = shapeCoordinate[11][0];
    vertices[11].shapeCoordinate[1] = shapeCoordinate[11][1];
    vertices[11].quadCoordinate[0] = rightCoordinate;
    vertices[11].quadCoordinate[1] = bottomCoordinate - radiusCoordinateHeight;

    // Set bottom row of 4 vertices.
    vertices[12].position[0] = 0.0f;
    vertices[12].position[1] = height;
    vertices[12].shapeCoordinate[0] = shapeCoordinate[12][0];
    vertices[12].shapeCoordinate[1] = shapeCoordinate[12][1];
    vertices[12].quadCoordinate[0] = leftCoordinate;
    vertices[12].quadCoordinate[1] = bottomCoordinate;
    vertices[13].position[0] = radius;
    vertices[13].position[1] = height;
    vertices[13].shapeCoordinate[0] = shapeCoordinate[13][0];
    vertices[13].shapeCoordinate[1] = shapeCoordinate[13][1];
    vertices[13].quadCoordinate[0] = leftCoordinate + radiusCoordinateWidth;
    vertices[13].quadCoordinate[1] = bottomCoordinate;
    vertices[14].position[0] = width - radius;
    vertices[14].position[1] = height;
    vertices[14].shapeCoordinate[0] = shapeCoordinate[14][0];
    vertices[14].shapeCoordinate[1] = shapeCoordinate[14][1];
    vertices[14].quadCoordinate[0] = rightCoordinate - radiusCoordinateWidth;
    vertices[14].quadCoordinate[1] = bottomCoordinate;
    vertices[15].position[0] = width;
    vertices[15].position[1] = height;
    vertices[15].shapeCoordinate[0] = shapeCoordinate[15][0];
    vertices[15].shapeCoordinate[1] = shapeCoordinate[15][1];
    vertices[15].quadCoordinate[0] = rightCoordinate;
    vertices[15].quadCoordinate[1] = bottomCoordinate;

    markDirty(DirtyGeometry);
}

// --- QtQuick item ---

struct ShapeTextures
{
    ShapeTextures() : high(0), low(0) {}
    QSGTexture* high;
    QSGTexture* low;
};

static QHash<QOpenGLContext*, ShapeTextures> shapeTexturesHash;

const float implicitGridUnitWidth = 8.0f;
const float implicitGridUnitHeight = 8.0f;

// Threshold in grid unit defining the texture quality to be used.
const float lowHighTextureThreshold = 11.0f;

/*!
    \qmltype UbuntuShape
    \instantiates UCUbuntuShape
    \inqmlmodule Ubuntu.Components 1.1
    \ingroup ubuntu
    \brief The UbuntuShape item provides a standard Ubuntu shaped rounded rectangle.

    The UbuntuShape is used where a rounded rectangle is needed either filled
    with a color or an image that it crops.

    When given with a \l color it is applied with an overlay blending as a
    vertical gradient going from \l color to \l gradientColor.
    Two corner \l radius are available, "small" (default) and "medium", that
    determine the size of the corners.
    Optionally, an Image can be passed that will be displayed inside the
    UbuntuShape and cropped to fit it.

    Examples:
    \qml
        import Ubuntu.Components 1.1

        UbuntuShape {
            color: "lightblue"
            radius: "medium"
        }
    \endqml

    \qml
        import Ubuntu.Components 1.1

        UbuntuShape {
            image: Image {
                source: "icon.png"
            }
        }
    \endqml
*/
UCUbuntuShape::UCUbuntuShape(QQuickItem* parent)
    : QQuickItem(parent)
    , image_(NULL)
    , source_(NULL)
    , sourceTextureProvider_(NULL)
    , color_(qRgba(0, 0, 0, 0))
    , gradientColor_(qRgba(0, 0, 0, 0))
    , backgroundColor_(qRgba(0, 0, 0, 0))
    , secondaryBackgroundColor_(qRgba(0, 0, 0, 0))
    , backgroundMode_(UCUbuntuShape::BackgroundColor)
    , radiusString_("small")
    , radius_(UCUbuntuShape::SmallRadius)
    , border_(UCUbuntuShape::IdleBorder)
    , hAlignment_(UCUbuntuShape::AlignHCenter)
    , vAlignment_(UCUbuntuShape::AlignVCenter)
    , gridUnit_(UCUnits::instance().gridUnit())
    , overlayX_(0)
    , overlayY_(0)
    , overlayWidth_(0)
    , overlayHeight_(0)
    , overlayColor_(qRgba(0, 0, 0, 0))
    , flags_(UCUbuntuShape::StretchedFlag)
{
    setFlag(ItemHasContents);
    QObject::connect(&UCUnits::instance(), SIGNAL(gridUnitChanged()), this,
                     SLOT(gridUnitChanged()));
    setImplicitWidth(implicitGridUnitWidth * gridUnit_);
    setImplicitHeight(implicitGridUnitHeight * gridUnit_);
    update();
}

/*!
 * \deprecated
 * \qmlproperty color UbuntuShape::color
 *
 * This property defines the color used to fill the UbuntuShape when there is no \image set. If \l
 * gradientColor is set, this property defines the top color of the gradient.
 *
 * \note Use \l backgroundColor, \l secondaryBackgroundColor and \l backgroundMode instead.
*/
void UCUbuntuShape::setColor(const QColor& color)
{
    const QRgb colorRgb = qRgba(color.red(), color.green(), color.blue(), color.alpha());
    if (color_ != colorRgb) {
        color_ = colorRgb;
        // gradientColor has the same value as color unless it was explicitly set.
        if (!(flags_ & UCUbuntuShape::GradientColorSetFlag)) {
            gradientColor_ = colorRgb;
            Q_EMIT gradientColorChanged();
        }
        if (!(flags_ & UCUbuntuShape::BackgroundApiSetFlag)) {
            update();
        }
        Q_EMIT colorChanged();
    }
}

/*!
 * \deprecated
 * \qmlproperty color UbuntuShape::gradientColor
 *
 * This property defines the bottom color used for the vertical gradient filling the UbuntuShape
 * when there is no \image set. As long as this property is not set, a single color (defined by \l
 * color) is used to fill the UbuntuShape.
 *
 * \note Use \l backgroundColor, \l secondaryBackgroundColor and \l backgroundMode instead.
 */
void UCUbuntuShape::setGradientColor(const QColor& gradientColor)
{
    flags_ |= UCUbuntuShape::GradientColorSetFlag;
    const QRgb gradientColorRgb = qRgba(
        gradientColor.red(), gradientColor.green(), gradientColor.blue(), gradientColor.alpha());
    if (gradientColor_ != gradientColorRgb) {
        gradientColor_ = gradientColorRgb;
        if (!(flags_ & UCUbuntuShape::BackgroundApiSetFlag)) {
            update();
        }
        Q_EMIT gradientColorChanged();
    }
}

/*!
    \qmlproperty string UbuntuShape::radius

    The size of the corners among: "small" (default) and "medium".
*/
void UCUbuntuShape::setRadius(const QString& radius)
{
    if (radiusString_ != radius) {
        radiusString_ = radius;
        radius_ = (radius == "medium") ? UCUbuntuShape::MediumRadius : UCUbuntuShape::SmallRadius;
        update();
        Q_EMIT radiusChanged();
    }
}

/*!
    \deprecated
    \qmlproperty url UbuntuShape::borderSource

    The image used as a border.
    We plan to expose that feature through styling properties.
*/
void UCUbuntuShape::setBorderSource(const QString& borderSource)
{
    if (borderSource_ != borderSource) {
        if (borderSource.endsWith(QString("radius_idle.sci")))
            border_ = UCUbuntuShape::IdleBorder;
        else if (borderSource.endsWith(QString("radius_pressed.sci")))
            border_ = UCUbuntuShape::PressedBorder;
        else
            border_ = UCUbuntuShape::RawBorder;
        borderSource_ = borderSource;
        update();
        Q_EMIT borderSourceChanged();
    }
}

/*!
 * \qmlproperty variant UbuntuShape::source
 *
 * This property holds the source \c Image or \c ShaderEffectSource rendered in the UbuntuShape. It
 * is blended over the \l backgroundColor. Default value is \c null.
 *
 * In the case of an \c {Image}-based source, the fill modes and alignments set on the \c Image are
 * not monitored, use the corresponding properties of the UbuntuShape instead. The only property
 * that is monitored on both \c Image and \c ShaderEffectSource sources is \c smooth.
 *
 * \qml
 *     UbuntuShape {
 *         source: Image { source: "ubuntu.png" }
 *     }
 * \endqml
 *
 * \note Setting this property disables the support for the deprecated properties \l image,
 *  \l horizontalAlignment, \l verticalAlignment and \l stretched.
 */
void UCUbuntuShape::setSource(const QVariant& source)
{
    QQuickItem* newSource = qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(source));
    if (source_ != newSource) {
        dropImageSupport();
        if (newSource && !newSource->parentItem()) {
            // Inlined images need a parent and must not be visible.
            newSource->setParentItem(this);
            newSource->setVisible(false);
        }
        update();
        source_ = newSource;
        Q_EMIT sourceChanged();
    }
}

/*!
 * \qmlproperty rect UbuntuShape::overlayGeometry
 *
 * This property defines the rectangle geometry (x, y, width, height) overlaying the UbuntuShape.
 * To disable the overlay, set \l overlayGeometry to the empty rectangle (x and/or y equal
 * 0). Default value is the empty rectangle.
 *
 * It is defined by a position and a size in normalized coordinates (in the range [0.0, 1.0]). An
 * overlay covering all the bottom part and starting from the middle of an UbuntuShape can be done
 * like this:
 *
 * \qml
 *     UbuntuShape {
 *         width: 200; height: 200
 *         overlayGeometry: Qt.rect(0.0, 0.5, 1.0, 0.5)
 *     }
 * \endqml
 *
 * Specifying a position and size in pixels can be done by dividing the values by the size. Here is
 * an example doing the same as the previous one:
 *
 * \qml
 *     UbuntuShape {
 *         width: 200; height: 200
 *         overlayGeometry: Qt.rect(100.0 / width, 100.0 / height,
 *                                  200.0 / width, 100.0 / height)
 *     }
 * \endqml
 *
 * \note The area potentially exceeding the UbuntuShape is cropped.
 */
void UCUbuntuShape::setOverlayGeometry(const QRectF& overlayGeometry)
{
    // Crop rectangle and convert to 16-bit unsigned integer.
    const float x = qMax(0.0f, qMin(1.0f, static_cast<float>(overlayGeometry.x())));
    float width = qMax(0.0f, static_cast<float>(overlayGeometry.width()));
    if ((x + width) > 1.0f) {
        width += 1.0f - (x + width);
    }
    const float y = qMax(0.0f, qMin(1.0f, static_cast<float>(overlayGeometry.y())));
    float height = qMax(0.0f, static_cast<float>(overlayGeometry.height()));
    if ((y + height) > 1.0f) {
        height += 1.0f - (y + height);
    }
    const quint16 overlayX = static_cast<quint16>(x * 0xffff);
    const quint16 overlayY = static_cast<quint16>(y * 0xffff);
    const quint16 overlayWidth = static_cast<quint16>(width * 0xffff);
    const quint16 overlayHeight = static_cast<quint16>(height * 0xffff);

    if ((overlayX_ != overlayX) || (overlayY_ != overlayY) ||
        (overlayWidth_ != overlayWidth) || (overlayHeight_ != overlayHeight)) {
        overlayX_ = overlayX;
        overlayY_ = overlayY;
        overlayWidth_ = overlayWidth;
        overlayHeight_ = overlayHeight;
        update();
        Q_EMIT overlayGeometryChanged();
    }
}

/*!
 * \qmlproperty color UbuntuShape::overlayColor
 *
 * This property defines the color of the rectangle overlaying the UbuntuShape. Default value is
 * transparent black.
 */
void UCUbuntuShape::setOverlayColor(const QColor& overlayColor)
{
    const QRgb overlayColorRgb = qRgba(
        overlayColor.red(), overlayColor.green(), overlayColor.blue(), overlayColor.alpha());
    if (overlayColor_ != overlayColorRgb) {
        overlayColor_ = overlayColorRgb;
        update();
        Q_EMIT overlayColorChanged();
    }
}

/*!
 * \qmlproperty color UbuntuShape::backgroundColor
 * \qmlproperty color UbuntuShape::secondaryBackgroundColor
 *
 * These properties define the background colors of the UbuntuShape. \c secondaryBackgroundColor is
 * used only when \l backgroundMode is set to \c VerticalGradient. Default value is transparent
 * black for both.
 *
 * \note Setting one of these properties disables the support for the deprecated properties \l color
 * and \l gradientColor.
 */
void UCUbuntuShape::setBackgroundColor(const QColor& backgroundColor)
{
    flags_ |= UCUbuntuShape::BackgroundApiSetFlag;
    const QRgb backgroundColorRgb = qRgba(
        backgroundColor.red(), backgroundColor.green(), backgroundColor.blue(),
        backgroundColor.alpha());
    if (backgroundColor_ != backgroundColorRgb) {
        backgroundColor_ = backgroundColorRgb;
        update();
        Q_EMIT backgroundColorChanged();
    }
}

void UCUbuntuShape::setSecondaryBackgroundColor(const QColor& secondaryBackgroundColor)
{
    flags_ |= UCUbuntuShape::BackgroundApiSetFlag;
    const QRgb secondaryBackgroundColorRgb = qRgba(
        secondaryBackgroundColor.red(), secondaryBackgroundColor.green(),
        secondaryBackgroundColor.blue(), secondaryBackgroundColor.alpha());
    if (secondaryBackgroundColor_ != secondaryBackgroundColorRgb) {
        secondaryBackgroundColor_ = secondaryBackgroundColorRgb;
        update();
        Q_EMIT secondaryBackgroundColorChanged();
    }
}

/*!
 * \qmlproperty enumeration UbuntuShape::backgroundMode
 *
 * This property defines the mode used by the UbuntuShape to render its background. Default value
 * is \c BackgroundColor.
 *
 * \list
 * \li UbuntuShape.BackgroundColor - background color is \l backgroundColor
 * \li UbuntuShape.VerticalGradient - background color is a vertical gradient from
 *     \l backgroundColor (top) to \l secondaryBackgroundColor (bottom)
 * \endlist
 *
 * \note Setting this properties disables the support for the deprecated properties \l color and \l
 * gradientColor.
 */
void UCUbuntuShape::setBackgroundMode(BackgroundMode backgroundMode)
{
    flags_ |= UCUbuntuShape::BackgroundApiSetFlag;
    if (backgroundMode_ != backgroundMode) {
        backgroundMode_ = backgroundMode;
        update();
        Q_EMIT backgroundModeChanged();
    }
}

/*!
 * \deprecated
 * \qmlproperty Image UbuntuShape::image
 *
 * This property holds the \c Image or \c ShaderEffectSource rendered in the UbuntuShape. In case of
 * an \c Image, it watches for fillMode (\c Image.PreserveAspectCrop), \c horizontalAlignment and \c
 * verticalAlignment property changes. Default value is \c null.
 *
 * \note Use \l source instead.
 */
void UCUbuntuShape::setImage(const QVariant& image)
{
    QQuickItem* newImage = qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(image));
    if (image_ != newImage) {
        QObject::disconnect(image_);
        if (!(flags_ & UCUbuntuShape::SourceApiSetFlag)) {
            if (newImage) {
                // Watch for property changes.
                updateFromImageProperties(newImage);
                connectToImageProperties(newImage);
                if (!newImage->parentItem()) {
                    // Inlined images need a parent and must not be visible.
                    newImage->setParentItem(this);
                    newImage->setVisible(false);
                }
            }
            update();
        }
        image_ = newImage;
        Q_EMIT imageChanged();
    }
}

void UCUbuntuShape::updateFromImageProperties(QQuickItem* image)
{
    int alignment;

    // UCUbuntuShape::stretched depends on Image::fillMode.
    QQuickImage::FillMode fillMode = (QQuickImage::FillMode)image->property("fillMode").toInt();
    if (fillMode == QQuickImage::PreserveAspectCrop) {
        setStretched(false);
    } else {
        setStretched(true);
    }

    // UCUbuntuShape::horizontalAlignment depends on Image::horizontalAlignment.
    int imageHorizontalAlignment = image->property("horizontalAlignment").toInt();
    if (imageHorizontalAlignment == Qt::AlignLeft) {
        alignment = UCUbuntuShape::AlignLeft;
    } else if (imageHorizontalAlignment == Qt::AlignRight) {
        alignment = UCUbuntuShape::AlignRight;
    } else {
        alignment = UCUbuntuShape::AlignHCenter;
    }
    setHorizontalAlignment(static_cast<UCUbuntuShape::HAlignment>(alignment));

    // UCUbuntuShape::verticalAlignment depends on Image::verticalAlignment.
    int imageVerticalAlignment = image->property("verticalAlignment").toInt();
    if (imageVerticalAlignment == Qt::AlignTop) {
        alignment = UCUbuntuShape::AlignTop;
    } else if (imageVerticalAlignment == Qt::AlignBottom) {
        alignment = UCUbuntuShape::AlignBottom;
    } else {
        alignment = UCUbuntuShape::AlignVCenter;
    }
    setVerticalAlignment(static_cast<UCUbuntuShape::VAlignment>(alignment));
}

void UCUbuntuShape::connectToPropertyChange(QObject* sender, const char* property,
                                            QObject* receiver, const char* slot)
{
    int propertyIndex = sender->metaObject()->indexOfProperty(property);
    if (propertyIndex != -1) {
        QMetaMethod changeSignal = sender->metaObject()->property(propertyIndex).notifySignal();
        int slotIndex = receiver->metaObject()->indexOfSlot(slot);
        QMetaMethod updateSlot = receiver->metaObject()->method(slotIndex);
        QObject::connect(sender, changeSignal, receiver, updateSlot);
    }
}

void UCUbuntuShape::connectToImageProperties(QQuickItem* image)
{
    connectToPropertyChange(image, "fillMode", this, "onImagePropertiesChanged()");
    connectToPropertyChange(image, "horizontalAlignment", this, "onImagePropertiesChanged()");
    connectToPropertyChange(image, "verticalAlignment", this, "onImagePropertiesChanged()");
}

void UCUbuntuShape::onImagePropertiesChanged()
{
    QQuickItem* image = qobject_cast<QQuickItem*>(sender());
    updateFromImageProperties(image);
}

void UCUbuntuShape::setStretched(bool stretched)
{
    if (!!(flags_ & UCUbuntuShape::StretchedFlag) != stretched) {
        if (stretched) {
            flags_ |= UCUbuntuShape::StretchedFlag;
        } else {
            flags_ &= ~UCUbuntuShape::StretchedFlag;
        }
        update();
        Q_EMIT stretchedChanged();
    }
}

void UCUbuntuShape::setHorizontalAlignment(HAlignment hAlignment)
{
    if (hAlignment_ != hAlignment) {
        hAlignment_ = hAlignment;
        update();
        Q_EMIT horizontalAlignmentChanged();
    }
}

void UCUbuntuShape::setVerticalAlignment(VAlignment vAlignment)
{
    if (vAlignment_ != vAlignment) {
        vAlignment_ = vAlignment;
        update();
        Q_EMIT verticalAlignmentChanged();
    }
}

void UCUbuntuShape::dropImageSupport()
{
    flags_ |= UCUbuntuShape::SourceApiSetFlag;
    if (image_) {
        QObject::disconnect(image_);
        image_ = NULL;
        Q_EMIT imageChanged();
    }
}

void UCUbuntuShape::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    update();
}

void UCUbuntuShape::onOpenglContextDestroyed()
{
    QOpenGLContext* context = qobject_cast<QOpenGLContext*>(sender());
    if (Q_UNLIKELY(!context)) {
        return;
    }

    QHash<QOpenGLContext*, ShapeTextures>::iterator it = shapeTexturesHash.find(context);
    if (it != shapeTexturesHash.end()) {
        ShapeTextures &shapeTextures = it.value();
        delete shapeTextures.high;
        delete shapeTextures.low;
        shapeTexturesHash.erase(it);
    }
}

void UCUbuntuShape::gridUnitChanged()
{
    gridUnit_ = UCUnits::instance().gridUnit();
    setImplicitWidth(implicitGridUnitWidth * gridUnit_);
    setImplicitHeight(implicitGridUnitHeight * gridUnit_);
    update();
}

void UCUbuntuShape::providerDestroyed(QObject* object)
{
    Q_UNUSED(object);
    sourceTextureProvider_ = NULL;
}

QSGNode* UCUbuntuShape::updatePaintNode(QSGNode* old_node, UpdatePaintNodeData* data)
{
    Q_UNUSED(data);

    // OpenGL allocates textures per context, so we store textures reused by all shape instances
    // per context as well.
    QOpenGLContext* openglContext = window() ? window()->openglContext() : NULL;
    if (Q_UNLIKELY(!openglContext)) {
        qCritical() << "Window has no OpenGL context!";
        delete old_node;
        return NULL;
    }
    ShapeTextures &shapeTextures = shapeTexturesHash[openglContext];
    if (!shapeTextures.high) {
        shapeTextures.high = window()->createTextureFromImage(
            QImage(shapeTextureHigh.data, shapeTextureHigh.width, shapeTextureHigh.height,
                   QImage::Format_ARGB32_Premultiplied));
        shapeTextures.low = window()->createTextureFromImage(
            QImage(shapeTextureLow.data, shapeTextureLow.width, shapeTextureLow.height,
                   QImage::Format_ARGB32_Premultiplied));
        QObject::connect(openglContext, SIGNAL(aboutToBeDestroyed()),
                         this, SLOT(onOpenglContextDestroyed()), Qt::DirectConnection);
    }

    ShapeNode* node = static_cast<ShapeNode*>(old_node);
    if (!node) {
        node = new ShapeNode(this);
    }
    ShapeMaterial::Data* materialData = node->material()->data();

    // Update the shape item whenever the source item's texture changes.
    const QQuickItem* source = (flags_ & UCUbuntuShape::SourceApiSetFlag) ? source_ : image_;
    QSGTextureProvider* provider = source ? source->textureProvider() : NULL;
    if (provider != sourceTextureProvider_) {
        if (sourceTextureProvider_) {
            QObject::disconnect(sourceTextureProvider_, SIGNAL(textureChanged()),
                                this, SLOT(update()));
            QObject::disconnect(sourceTextureProvider_, SIGNAL(destroyed()),
                                this, SLOT(providerDestroyed()));
        }
        if (provider) {
            QObject::connect(provider, SIGNAL(textureChanged()), this, SLOT(update()));
            QObject::connect(provider, SIGNAL(destroyed()), this, SLOT(providerDestroyed()));
        }
        sourceTextureProvider_ = provider;
    }

    TextureData* textureData;
    if (gridUnit_ > lowHighTextureThreshold) {
        textureData = &shapeTextureHigh;
        materialData->shapeTexture = shapeTextures.high;
    } else {
        textureData = &shapeTextureLow;
        materialData->shapeTexture = shapeTextures.low;
    }

    // Set the shape texture to be used by the materials depending on current grid unit. The radius
    // is set considering the current grid unit and the texture raster grid unit. When the item size
    // is less than 2 radii, the radius is scaled down.
    float radius = (radius_ == UCUbuntuShape::SmallRadius) ?
        textureData->smallRadius : textureData->mediumRadius;
    const float scaleFactor = gridUnit_ / textureData->gridUnit;
    materialData->shapeTextureFiltering = QSGTexture::Nearest;
    if (scaleFactor != 1.0f) {
        radius *= scaleFactor;
        materialData->shapeTextureFiltering = QSGTexture::Linear;
    }
    const float geometryWidth = width();
    const float geometryHeight = height();
    const float halfMinWidthHeight = qMin(geometryWidth, geometryHeight) * 0.5f;
    if (radius > halfMinWidthHeight) {
        radius = halfMinWidthHeight;
        materialData->shapeTextureFiltering = QSGTexture::Linear;
    }

    quint8 flags = 0;

    // Update background material data.
    if (flags_ & UCUbuntuShape::BackgroundApiSetFlag) {
        // BackgroundApiSetFlag is flagged as soon as one of the background property API is set. It
        // allows to keep the support for the deprecated color and gradientColor properties.
        quint32 a = qAlpha(backgroundColor_);
        materialData->backgroundColor = qRgba(
            (qRed(backgroundColor_) * a) / 255, (qGreen(backgroundColor_) * a) / 255,
            (qBlue(backgroundColor_) * a) / 255, a);
        const QRgb color = (backgroundMode_ == UCUbuntuShape::BackgroundColor) ?
            backgroundColor_ : secondaryBackgroundColor_;
        a = qAlpha(color);
        materialData->secondaryBackgroundColor = qRgba(
            (qRed(color) * a) / 255, (qGreen(color) * a) / 255, (qBlue(color) * a) / 255, a);
    } else {
        quint32 a = qAlpha(color_);
        materialData->backgroundColor = qRgba(
            (qRed(color_) * a) / 255, (qGreen(color_) * a) / 255, (qBlue(color_) * a) / 255, a);
        a = qAlpha(gradientColor_);
        materialData->secondaryBackgroundColor = qRgba(
            (qRed(gradientColor_) * a) / 255, (qGreen(gradientColor_) * a) / 255,
            (qBlue(gradientColor_) * a) / 255, a);
    }

    // Update image material data.
    if (provider && provider->texture()) {
        const QRectF subRect = provider->texture()->normalizedTextureSubRect();
        materialData->sourceTextureProvider = provider;
        materialData->atlasTransform[0] = static_cast<quint16>(subRect.width() * 0xffff);
        materialData->atlasTransform[1] = static_cast<quint16>(subRect.height() * 0xffff);
        materialData->atlasTransform[2] = static_cast<quint16>(subRect.x() * 0xffff);
        materialData->atlasTransform[3] = static_cast<quint16>(subRect.y() * 0xffff);
        flags |= ShapeMaterial::Data::TexturedFlag;
    } else {
        materialData->sourceTextureProvider = NULL;
        materialData->atlasTransform[0] = 0;
        materialData->atlasTransform[1] = 0;
        materialData->atlasTransform[2] = 0;
        materialData->atlasTransform[3] = 0;
    }

    // Update overlay material data.
    if ((overlayWidth_ != 0) && (overlayHeight_ != 0)) {
        const quint32 a = qAlpha(overlayColor_);
        materialData->overlayColor = qRgba(
            (qRed(overlayColor_) * a) / 255, (qGreen(overlayColor_) * a) / 255,
            (qBlue(overlayColor_) * a) / 255, a);
        materialData->overlaySteps[0] = overlayX_;
        materialData->overlaySteps[1] = overlayY_;
        materialData->overlaySteps[2] = overlayX_ + overlayWidth_;
        materialData->overlaySteps[3] = overlayY_ + overlayHeight_;
        flags |= ShapeMaterial::Data::OverlaidFlag;
    } else {
        // Overlay data has to be set to 0 so that shapes with different values can be batched
        // together (ShapeMaterial::compare() uses memcmp()).
        materialData->overlayColor = qRgba(0, 0, 0, 0);
        materialData->overlaySteps[0] = 0;
        materialData->overlaySteps[1] = 0;
        materialData->overlaySteps[2] = 0;
        materialData->overlaySteps[3] = 0;
    }

    materialData->flags = flags;

    // Update vertices and material.
    int index = (border_ == UCUbuntuShape::RawBorder) ?
        0 : (border_ == UCUbuntuShape::IdleBorder) ? 1 : 2;
    if (radius_ == UCUbuntuShape::SmallRadius)
        index += 3;
    node->setVertices(geometryWidth, geometryHeight, radius, source,
                      !!(flags_ & UCUbuntuShape::StretchedFlag), hAlignment_, vAlignment_,
                      textureData->coordinate[index]);

    return node;
}
