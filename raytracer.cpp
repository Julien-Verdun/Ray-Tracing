#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define M_PI 3.14159265358979323846

class Vector
{
public:
    explicit Vector(double x = 0, double y = 0, double z = 0)
    {
        coords[0] = x;
        coords[1] = y;
        coords[2] = z;
    }
    double operator[](int i) const { return coords[i]; };
    double &operator[](int i) { return coords[i]; };
    double sqrNorm()
    {
        return coords[0] * coords[0] + coords[1] * coords[1] + coords[2] * coords[2];
    }
    Vector get_normalized()
    {
        double n = sqrt(sqrNorm());
        return Vector(coords[0] / n, coords[1] / n, coords[2] / n);
    }

private:
    double coords[3];
};

Vector operator+(const Vector &a, const Vector &b)
{
    return Vector(a[0] + b[0], a[1] + b[1], a[2] + b[2]);
}

Vector operator-(const Vector &a, const Vector &b)
{
    return Vector(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
}

Vector operator*(double a, const Vector &b)
{
    return Vector(a * b[0], a * b[1], a * b[2]);
}

Vector operator*(const Vector &a, double b)
{
    return Vector(a[0] * b, a[1] * b, a[2] * b);
}

Vector operator/(const Vector &a, double b)
{
    return Vector(a[0] / b, a[1] / b, a[2] / b);
}

double dot(const Vector &a, const Vector &b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

class Ray
{
public:
    Ray(const Vector &C, const Vector &u) : C(C), u(u)
    {
    }

    Vector C, u;
};

class Sphere
{
public:
    Sphere(const Vector &O, double R, const Vector &albedo) : O(O), R(R), albedo(albedo)
    {
    }
    bool intersect(const Ray &r, Vector &P, Vector &N, double &t)
    {
        // solves a*t^2 + b*t + c = 0
        double a = 1;
        double b = 2 * dot(r.u, r.C - O);
        double c = (r.C - O).sqrNorm() - R * R;
        double delta = b * b - 4 * a * c;

        if (delta < 0)
        {
            return false;
        }
        double sqDelta = sqrt(delta);
        double t2 = (-b + sqDelta) / (2 * a);

        if (t2 < 0)
            return false;

        // double t;
        double t1 = (-b - sqDelta) / (2 * a);

        if (t1 > 0)
        {
            t = t1;
        }
        else
        {
            t = t2;
        }

        P = r.C + t * r.u;
        N = (P - O).get_normalized();

        return true;
    }
    Vector O;
    double R;
    Vector albedo;
};

class Scene
{
public:
    Scene(){};
    std::vector<Sphere> objects;
    bool intersect(const Ray &r, Vector &P, Vector &N, Vector &albedo)
    {
        double t = 1E10;
        bool has_inter = false;
        for (int i = 0; i < objects.size(); i++)
        {
            Vector localP, localN;
            double localt;
            if (objects[i].intersect(r, localP, localN, localt) && localt < t)
            {
                t = localt;
                has_inter = true;
                albedo = objects[i].albedo;
                P = localP;
                N = localN;
            }
        }
        return has_inter;
    };
};

int main()
{
    int W = 512;
    int H = 512;

    Vector C(0, 0, 55);
    Scene scene;
    // Vector O(0, 0, 0);
    // double R = 10;
    Sphere S1(Vector(-15, 0, 0), 10, Vector(0., 0., 1.));
    Sphere S2(Vector(0, 0, 0), 10, Vector(1., 1., 1.));
    Sphere S3(Vector(15, 0, 0), 10, Vector(1., 0., 0.));
    Sphere Smurga(Vector(-1000, 0, 0), 970, Vector(0., 0., 1.));
    Sphere Smurdr(Vector(1000, 0, 0), 970, Vector(1., 0., 0.));
    Sphere Smurfa(Vector(0, 0, -1000), 940, Vector(0., 1., 0.));
    Sphere Smurde(Vector(0, 0, 1000), 940, Vector(1., 0., 1.));
    Sphere Ssol(Vector(0, -1000, 0), 990, Vector(1., 1., 1.));
    Sphere Splafond(Vector(0, 1000, 0), 990, Vector(1., 1., 1.));
    scene.objects.push_back(S1);
    scene.objects.push_back(S2);
    scene.objects.push_back(S3);
    scene.objects.push_back(Smurga);
    scene.objects.push_back(Smurdr);
    scene.objects.push_back(Smurfa);
    scene.objects.push_back(Smurde);
    scene.objects.push_back(Ssol);
    // scene.objects.push_back(Splafond);

    double fov = 60 * M_PI / 180;
    double I = 1E7;
    // Vector rho(1, 0, 0);
    Vector L(-10, 20, 40);

    std::vector<unsigned char> image(W * H * 3, 0);
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            Vector u(j - W / 2, i - H / 2, -W / (2. * tan(fov / 2)));
            u = u.get_normalized();
            Ray r(C, u);
            Vector P, N, albedo;
            bool inter = scene.intersect(r, P, N, albedo);
            Vector color(0, 0, 0);
            if (inter)
            {
                Vector PL = L - P;
                double d = sqrt(PL.sqrNorm());
                // double ddot = dot(N, PL / d);
                // color = Vector(255, 255, 255);
                color = (I / (4 * M_PI * d * d)) * (albedo / M_PI) * std::max(0., dot(N, PL / d));
            }

            image[((H - i - 1) * W + j) * 3 + 0] = std::min(255., color[0]);
            image[((H - i - 1) * W + j) * 3 + 1] = std::min(255., color[1]);
            image[((H - i - 1) * W + j) * 3 + 2] = std::min(255., color[2]);
        }
    }
    stbi_write_png("raytracer.png", W, H, 3, &image[0], 0);

    return 0;
};

// run command line
// g++ raytracer.cpp -o raytraycer.exe