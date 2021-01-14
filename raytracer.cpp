#define _CRT_SECURE_NO_WARNINGS 1
#include <vector>

#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ctime>
#include <fstream>

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

Vector operator-(const Vector &a)
{
    return Vector(-a[0], -a[1], -a[2]);
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

double sqr(double x)
{
    return x * x;
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
    Sphere(const Vector &O, double R, const Vector &albedo, bool isMirror = false, bool isTransparent = false) : O(O), R(R), albedo(albedo), isMirror(isMirror), isTransparent(isTransparent)
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
    bool isMirror, isTransparent;
};

class Scene
{
public:
    Scene(){};
    std::vector<Sphere> objects;
    Vector L;
    double I;
    bool intersect(const Ray &r, Vector &P, Vector &N, Vector &albedo, bool &mirror, bool &transp, double &t)
    {
        t = 1E10;
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
                mirror = objects[i].isMirror;
                transp = objects[i].isTransparent;
            }
        }
        return has_inter;
    };

    Vector getColor(const Ray &r, int rebond)
    {
        double epsilon = 0.00001;
        Vector P, N, albedo;
        double t;
        bool mirror, transp;
        bool inter = intersect(r, P, N, albedo, mirror, transp, t);
        Vector color(0, 0, 0);
        if (rebond > 5)
            return Vector(0., 0., 0.);
        if (inter)
        {
            if (mirror)
            {
                Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                Ray reflectedRay(P + epsilon * N, reflectedDir);
                return getColor(reflectedRay, rebond + 1);
            }
            else
            {
                if (transp)
                {
                    double n1 = 1, n2 = 1.4;
                    Vector N2 = N;
                    if (dot(r.u, N) > 0)
                    { //on sort de la sphère
                        std::swap(n1, n2);
                        N2 = -N;
                    }

                    double rad = 1 - sqr(n1 / n2) * (1 - sqr(dot(r.u, N2)));

                    if (rad < 0) //rayon rasant, i-e rayon reflechie uniquement
                    {
                        Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                        Ray reflectedRay(P + epsilon * N, reflectedDir);
                        return getColor(reflectedRay, rebond + 1);
                    }

                    double k0 = sqr(n1 - n2) / sqr(n1 + n2);

                    double R = k0 + (1 - k0) * sqr(sqr(1 - std::abs(dot(N2, r.u)))) * (1 - std::abs(dot(N2, r.u)));

                    // implementation de la transmission de Fresnel avec tirage aleatoire et moyenne
                    // double random = rand() % 100;

                    // if (random <= 100 * R)
                    // {
                    //     Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                    //     Ray reflectedRay(P + epsilon * N, reflectedDir);
                    //     return getColor(reflectedRay, rebond + 1);
                    // }
                    // else
                    // {
                    //     // double T = 1 - R;
                    //     Vector tT = n1 / n2 * (r.u - dot(r.u, N2) * N2);
                    //     Vector tN = -sqrt(rad) * N2;
                    //     Vector refractedDir = tT + tN;
                    //     return getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1);
                    // }

                    Vector reflectedDir = r.u - 2 * dot(r.u, N) * N;
                    Ray reflectedRay(P + epsilon * N, reflectedDir);

                    double T = 1 - R;

                    Vector tT = n1 / n2 * (r.u - dot(r.u, N2) * N2);
                    Vector tN = -sqrt(rad) * N2;
                    Vector refractedDir = tT + tN;

                    // sans transmission de Fresnel
                    // return getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1);

                    // avec transmission de Fresnel
                    return (T * getColor(Ray(P - epsilon * N2, refractedDir), rebond + 1) + R * getColor(reflectedRay, rebond + 1));
                }
                else
                {
                    Vector PL = L - P;
                    double d = sqrt(PL.sqrNorm());
                    Vector shadowP, shadowN, shadowAlbedo;
                    double shadowt;
                    bool shadowMirror, shadowTransp;
                    Ray shadowRay(P + 0.00001 * N, PL / d);
                    bool shadowInter = intersect(shadowRay, shadowP, shadowN, shadowAlbedo, shadowMirror, shadowTransp, shadowt);
                    if (shadowInter && shadowt < d)
                    {
                        color = Vector(0., 0., 0.);
                    }
                    else
                    {
                        color = (I / (4 * M_PI * d * d)) * (albedo / M_PI) * std::max(0., dot(N, PL / d));
                    }
                }
            }
        }
        return color;
    }
};

int main()
{
    float ini_time = clock();
    int W = 512;
    int H = 512;

    Vector C(0, 0, 55);
    Scene scene;
    Sphere S1(Vector(-15, 0, 0), 10, Vector(0., 0., 1.));
    Sphere S2(Vector(0, 0, 0), 10, Vector(1., 1., 1.), true);
    Sphere S3(Vector(15, 0, 0), 10, Vector(1., 0., 0.), false, true);
    Sphere Smurga(Vector(-1000, 0, 0), 970, Vector(0., 0., 1.));
    Sphere Smurdr(Vector(1000, 0, 0), 970, Vector(1., 0., 0.));
    Sphere Smurfa(Vector(0, 0, -1000), 940, Vector(0., 1., 0.));
    Sphere Smurde(Vector(0, 0, 1000), 940, Vector(1., 0., 1.));
    Sphere Ssol(Vector(0, -1000, 0), 990, Vector(1., 1., 1.), true);
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
    scene.I = 5E9;
    scene.L = Vector(-10, 20, 40);

    std::vector<unsigned char> image(W * H * 3, 0);
    for (int i = 0; i < H; i++)
    {
        for (int j = 0; j < W; j++)
        {
            Vector u(j - W / 2, i - H / 2, -W / (2. * tan(fov / 2)));
            u = u.get_normalized();
            Ray r(C, u);

            Vector color = scene.getColor(r, 0);

            // implementation de la transmission de Fresnel avec tirage aleatoire et moyenne
            // Vector color = Vector(0., 0., 0.);
            // int nbVal = 30;
            // for (int i = 0; i < nbVal; i++)
            // {
            //     color = color + scene.getColor(r, 0);
            // }
            // color = color / nbVal;

            image[((H - i - 1) * W + j) * 3 + 0] = std::min(255., std::pow(color[0], 0.45));
            image[((H - i - 1) * W + j) * 3 + 1] = std::min(255., std::pow(color[1], 0.45));
            image[((H - i - 1) * W + j) * 3 + 2] = std::min(255., std::pow(color[2], 0.45));
        }
    }

    // wwriting the image on a png file
    stbi_write_png("raytracer.png", W, H, 3, &image[0], 0);

    // writing execution time on a txt file
    std::ofstream execution_file;
    execution_file.open("execution_time.txt");
    execution_file << (clock() - ini_time) / CLOCKS_PER_SEC;
    execution_file.close();

    return 0;
};

// run command line
// g++ raytracer.cpp -o raytraycer.exe