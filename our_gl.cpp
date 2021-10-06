
#include "our_gl.h"
#define PI 3.14159
#define a2r(x) (PI / 180 * x)

void modelView(Vec3f location, Vec3f rotation)
{
    Matrix locat = Matrix::identity();
    Matrix r_x = Matrix::identity();
    Matrix r_y = Matrix::identity();
    Matrix r_z = Matrix::identity();

    r_x[1][1] = cos(a2r(rotation[0]));
    r_x[1][2] = -sin(a2r(rotation[0]));
    r_x[2][1] = sin(a2r(rotation[0]));
    r_x[2][2] = cos(a2r(rotation[0]));

    r_y[0][0] = cos(a2r(rotation[1]));
    r_y[0][2] = sin(a2r(rotation[1]));
    r_y[2][0] = -sin(a2r(rotation[1]));
    r_y[2][2] = cos(a2r(rotation[1]));

    r_z[0][0] = cos(a2r(rotation[2]));
    r_z[0][1] = -sin(a2r(rotation[2]));
    r_z[1][0] = sin(a2r(rotation[2]));
    r_z[1][1] = cos(a2r(rotation[2]));

    Matrix rotate = r_z * r_y * r_x;

    locat[0][3] = location[0];
    locat[1][3] = location[1];
    locat[2][3] = location[2];

    ModelView = rotate * locat;
}

void perspective(float near, float far, float fov, float aspect)
{
    float n, f;

    n = near;
    f = far;

    Matrix m_persp = Matrix::identity();

    m_persp[0][0] = n;
    m_persp[1][1] = n;
    m_persp[2][2] = n + f;
    m_persp[2][3] = -(n * f);
    m_persp[3][2] = 1;
    m_persp[3][3] = 0;

    //ndcView(near, far, fov, aspect);
    Perspective = m_persp;
}

void ndcView(float near, float far, float fov, float aspect)
{
    fov = PI / 180 * (fov / 2);

    float n, f, l, r, t, b;

    n = near;
    f = far;
    t = -tan(fov) * n;
    b = -t;
    r = aspect * t;
    l = -r;

    Matrix m_scale = Matrix::identity();
    Matrix m_location = Matrix::identity();

    m_scale[0][0] = 2 / (r - l);
    m_scale[1][1] = 2 / (t - b);
    m_scale[2][2] = 2 / (n - f);

    m_location[0][3] = -(r + l) / 2;
    m_location[1][3] = -(t + b) / 2;
    m_location[2][3] = -(n + f) / 2;

    NDCView = m_scale * m_location;
}

void orthographic(float near, float far, float fov, float aspect, float width)
{
    fov = PI / 180 * (fov / 2);

    float n, f, l, r, t, b;

    n = near;
    f = far;
    t = -tan(fov) * n;
    b = -t;
    r = aspect * t;
    l = -r;

    Matrix m_scale = Matrix::identity();
    Matrix m_location = Matrix::identity();
    Matrix m_camera = Matrix::identity();

    m_scale[0][0] = 2 / (r - l);
    m_scale[1][1] = 2 / (t - b);
    m_scale[2][2] = 2 / (n - f);

    m_location[0][3] = -(r + l) / 2;
    m_location[1][3] = -(t + b) / 2;
    m_location[2][3] = -(n + f) / 2;

    m_camera[0][0] = 1 / width;
    m_camera[1][1] = 1 / width;

    Orthographic = m_scale * m_camera * m_location;


}

void cameraView(Vec3f location, Vec3f rotation)
{
    Vec4f x_axis = embed<4>(Vec3f(1, 0, 0));
    Vec4f y_axis = embed<4>(Vec3f(0, 1, 0));
    Vec4f z_axis = embed<4>(Vec3f(0, 0, 1));

    Matrix t_view = Matrix::identity();
    Matrix r_view = Matrix::identity();
    Matrix r_x = Matrix::identity();
    Matrix r_y = Matrix::identity();
    Matrix r_z = Matrix::identity();

    r_x[1][1] = cos(a2r(rotation[0]));
    r_x[1][2] = -sin(a2r(rotation[0]));
    r_x[2][1] = sin(a2r(rotation[0]));
    r_x[2][2] = cos(a2r(rotation[0]));

    r_y[0][0] = cos(a2r(rotation[1]));
    r_y[0][2] = sin(a2r(rotation[1]));
    r_y[2][0] = -sin(a2r(rotation[1]));
    r_y[2][2] = cos(a2r(rotation[1]));

    r_z[0][0] = cos(a2r(rotation[2]));
    r_z[0][1] = -sin(a2r(rotation[2]));
    r_z[1][0] = sin(a2r(rotation[2]));
    r_z[1][1] = cos(a2r(rotation[2]));

    Matrix rotate = r_z * r_y * r_x;

    x_axis = rotate * x_axis;
    y_axis = rotate * y_axis;
    z_axis = rotate * z_axis;

    t_view[0][3] = -location[0];
    t_view[1][3] = -location[1];
    t_view[2][3] = -location[2];

    r_view[0][0] = -x_axis[0];
    r_view[0][1] = -x_axis[1];
    r_view[0][2] = -x_axis[2];
    r_view[1][0] = y_axis[0];
    r_view[1][1] = y_axis[1];
    r_view[1][2] = y_axis[2];
    r_view[2][0] = -z_axis[0];
    r_view[2][1] = -z_axis[1];
    r_view[2][2] = -z_axis[2];

    CameraView = r_view * t_view;
}

void viewport(int width, int height)
{
    Viewport = Matrix::identity();
    Viewport[0][0] = width / 2;
    Viewport[1][1] = height / 2;
    Viewport[0][3] = width / 2 + 0.5f;
    Viewport[1][3] = height / 2 + 0.5f;
}

Vec3f barycentricCoord(const Vec4f* vertex, const Vec2f& point)
{
    float alpha = (-(point.x - vertex[1][0]) * (vertex[2][1] - vertex[1][1]) + (point.y - vertex[1][1]) * (vertex[2][0] - vertex[1][0])) / (-(vertex[0][0] - vertex[1][0]) * (vertex[2][1] - vertex[1][1]) + (vertex[0][1] - vertex[1][1]) * (vertex[2][0] - vertex[1][0]));
    float beta = (-(point.x - vertex[2][0]) * (vertex[0][1] - vertex[2][1]) + (point.y - vertex[2][1]) * (vertex[0][0] - vertex[2][0])) / (-(vertex[1][0] - vertex[2][0]) * (vertex[0][1] - vertex[2][1]) + (vertex[1][1] - vertex[2][1]) * (vertex[0][0] - vertex[2][0]));
    float gama = 1 - alpha - beta;
    return Vec3f(alpha, beta, gama);
}

void triangle(const Vec4f* vertex, IShader& shader, TGAImage& image, zbuffer& zbuffer)
{
    int x_boundingBoxMax = (int)(std::max((std::max(vertex[0][0], vertex[1][0])), vertex[2][0]));
    int x_boundingBoxMin = (int)(std::min((std::min(vertex[0][0], vertex[1][0])), vertex[2][0]));
    int y_boundingBoxMax = (int)(std::max((std::max(vertex[0][1], vertex[1][1])), vertex[2][1]));
    int y_boundingBoxMin = (int)(std::min((std::min(vertex[0][1], vertex[1][1])), vertex[2][1]));


    for (int x = x_boundingBoxMin; x <= x_boundingBoxMax; x++)
    {
        if (x<0 || x > image.get_width())
            continue;


        for (int y = y_boundingBoxMin; y <= y_boundingBoxMax; y++)
        {
            if (y<0 || y > image.get_height())
                continue;

            Vec3f bc = barycentricCoord(vertex, Vec2f(x, y));
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;// 跳过不在三角形内的点
            bc.x /= vertex[0][3];
            bc.y /= vertex[1][3];
            bc.z /= vertex[2][3];

            float zn = 1 / (bc[0] + bc[1] + bc[2]);

            float zOrder = (vertex[0][2] * bc.x + vertex[1][2] * bc.y + vertex[2][2] * bc.z) * zn;

            if (zOrder < zbuffer.get(x, y)) continue;

            TGAColor color;

            if (shader.fragment(bc, color))
            {
                zbuffer.set(x, y, zOrder);
                image.set(x, y, color);
            }
        }
    }
}