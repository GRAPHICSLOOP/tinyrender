#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "our_gl.h"

float* depthBuffer = NULL;

Matrix ModelView = Matrix::identity();
Matrix CameraView = Matrix::identity();
Matrix ShadowView = Matrix::identity();
Matrix Viewport = Matrix::identity();
Matrix Perspective = Matrix::identity();
Matrix NDCView = Matrix::identity();
Matrix Orthographic = Matrix::identity();

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor block = TGAColor(0, 0, 0, 255);

Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f lightDir(0, 0, -1);

struct ZBufferShader : IShader
{
    Vec4f vertex_pos[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f ver = embed<4>(model->vert(iface, nthvert));
        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / temp;
        ver[3] = temp;

        vertex_pos[nthvert] = ver;
        return ver;
    }

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        Vec4f fragmentCoord = (vertex_pos[0] * barycentricCoord[0] + vertex_pos[1] * barycentricCoord[1] + vertex_pos[2] * barycentricCoord[2]) * zn;

        float bar_z = (1 + fragmentCoord[2]);
        color = TGAColor(255, 255, 255, 255) * bar_z;

        return true;
    }

};

struct ShadowShader : IShader
{
    zbuffer* ShadowBuffer;
    Vec4f vertex_pos[3];
    Matrix shadowMatrix;
    Vec2f uv[3];

    virtual Vec4f vertex(int iface, int nthvert)
    {
        Vec4f ver = embed<4>(model->vert(iface, nthvert));
        uv[nthvert] = model->uv(iface, nthvert);
        ver = Viewport * NDCView * Perspective * CameraView * ModelView * ver;
        float temp = ver[3];
        ver = ver / temp;
        ver[3] = temp;
        vertex_pos[nthvert] = ver;
        return ver;
    }

    virtual bool fragment(Vec3f barycentricCoord, TGAColor& color)
    {
        float zn = 1 / (barycentricCoord[0] + barycentricCoord[1] + barycentricCoord[2]);
        Vec2f bar_uv = (uv[0] * barycentricCoord[0] + uv[1] * barycentricCoord[1] + uv[2] * barycentricCoord[2]) * zn;
        

        Vec4f fragmentCoord = (vertex_pos[0] * barycentricCoord[0] + vertex_pos[1] * barycentricCoord[1] + vertex_pos[2] * barycentricCoord[2]) * zn;
        float temp = fragmentCoord[3];
        fragmentCoord = fragmentCoord * temp;
        fragmentCoord[3] = temp;

        Vec4f shadowFragment = shadowMatrix * fragmentCoord;
        temp = shadowFragment[3];
        shadowFragment = shadowFragment / temp;
        shadowFragment[3] = temp;

        float fragmentZBuffer = shadowFragment[2];
        Vec2i index = Vec2i(shadowFragment[0]+0.0001, shadowFragment[1] + 0.0001); // 这里加上一点偏移是为了确保索引尽量正确，因为数据转换之后有一点丢失
        float bia = 0.009;
        float shadowbufferzbuffer = ShadowBuffer->get(index[0], index[1]) - bia;
        float shadow = .5 + .5 * (shadowbufferzbuffer < fragmentZBuffer); // 0.5 代表阴影的时候
        color = model->diffuse(bar_uv) * shadow;

        return true;
    }
};

int main(int argc, char** argv) {
    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head/african_head.obj");
    }

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage ShaderBuffer(width, height, TGAImage::RGB);
    lightDir.normalize();
    Vec4f vertex[3];

    ZBufferShader ZBufferShader;
    cameraView(Vec3f(0, 4, 4), Vec3f(45, 180, 0));
    perspective(-1, -10.f, 45, 1);
    ndcView(-1, -10.f, 45, 1);
    viewport(width, height);

    zbuffer shadowBuffer(width, height);
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            vertex[j] = ZBufferShader.vertex(i, j);
        }
        triangle(vertex, ZBufferShader, ShaderBuffer, shadowBuffer);
    }
    ShaderBuffer.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    ShaderBuffer.write_tga_file("ZBuffer.tga");
    Matrix ShadowMatrix = Viewport * NDCView * Perspective * CameraView * ModelView;


    zbuffer zbuffer(width, height);
    modelView(Vec3f(0, 0, 0), Vec3f(0, 0, 0));
    cameraView(Vec3f(0, 0, 4), Vec3f(0, 180, 0));
    perspective(-1, -10.f, 45, 1);
    ndcView(-1, -10.f, 45, 1);
    viewport(width, height);

    ShadowShader ShadowShader;
    ShadowShader.ShadowBuffer = &shadowBuffer;
    ShadowShader.shadowMatrix = ShadowMatrix * ((Viewport * NDCView * Perspective * CameraView * ModelView).invert());
    for (int i = 0; i < model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j++)
        {
            vertex[j] = ShadowShader.vertex(i, j);
        }
        triangle(vertex, ShadowShader, image, zbuffer);
    }


    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}
