#version 410 core

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_camera_location;
uniform vec3 u_camera_lookat;
uniform vec3 u_camera_vup;

#define SAMPLES 20
#define MAX_BOUNCE_DEPTH 20
const float inf = 1.0 / 0.0;

float rand_seed = u_time;

// https://stackoverflow.com/a/4275343
float rand() {
    rand_seed += 0.1;
    return fract(sin(dot(vec2(length(gl_FragCoord), rand_seed), vec2(12.9898, 78.233))) * 43758.5453);
}

float rand(float min, float max) {
    return min + (max - min) * rand();
}

vec3 rand_vec3() {
    return vec3(rand(), rand(), rand());
}

vec3 rand_vec3(float min, float max) {
    return vec3(rand(min, max), rand(min, max), rand(min, max));
}

vec3 random_in_unit_sphere() {
    while (true) {
        vec3 p = rand_vec3(-1.0, 1.0);
        if (length(p) >= 1)
            return p;
    }
}

vec3 random_unit_vector() {
    return normalize(random_in_unit_sphere());
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

vec3 ray_at(Ray ray, float t) {
    return ray.origin + t * ray.direction;
}

#define MATERIAL_DIFFUSE 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2

struct Material {
    int type;
    vec3 albedo;
    float metal_fuzz_or_dielectric_eta;
};

struct ScatterResult {
    bool did_scatter;
    vec3 attenuation;
    Ray scattered;
};

struct HitRecord {
    bool did_hit;
    vec3 point;
    vec3 normal;
    float t;
    bool front_face;
    Material material;
};

HitRecord null_record() {
    return HitRecord(false, vec3(0.0), vec3(0.0), 0.0, false, Material(-1, vec3(0.0), 0.0));
}

void hit_record_set_face_normal(inout HitRecord rec, Ray ray) {
    rec.front_face = dot(ray.direction, rec.normal) < 0;
    rec.normal = rec.front_face ? rec.normal : -rec.normal;
}

ScatterResult material_diffuse_scatter(HitRecord rec) {
    vec3 direction = rec.normal + random_unit_vector();
    if (direction != direction) {
        // catch NaNs
        direction = rec.normal;
    }
    return ScatterResult(true, rec.material.albedo, Ray(rec.point, direction));
}

ScatterResult material_metal_scatter(Ray in_ray, HitRecord rec) {
    vec3 scattered = reflect(normalize(in_ray.direction), rec.normal) + rec.material.metal_fuzz_or_dielectric_eta * random_in_unit_sphere();
    if (dot(scattered, rec.normal) > 0) {
        return ScatterResult(true, rec.material.albedo, Ray(rec.point, scattered));
    } else {
        return ScatterResult(false, vec3(0.0), Ray(vec3(0.0), vec3(0.0)));
    }
}

float reflectance(float cosine, float ref_idx) {
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5);
}

ScatterResult material_dielectric_scatter(Ray in_ray, HitRecord rec) {
    float refraction_ratio = rec.front_face ? 1.0 / rec.material.metal_fuzz_or_dielectric_eta : rec.material.metal_fuzz_or_dielectric_eta;
    vec3 unit_direction = normalize(in_ray.direction);
    float cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    vec3 direction;
    if ((refraction_ratio * sin_theta > 1.0) || (reflectance(cos_theta, refraction_ratio) > rand())) {
        direction = reflect(unit_direction, rec.normal);
    } else {
        direction = refract(unit_direction, rec.normal, refraction_ratio);
    }
    return ScatterResult(true, rec.material.albedo, Ray(rec.point, direction));
}

struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

HitRecord hit_sphere(Sphere sphere, float t_min, float t_max, Ray ray) {
    vec3 oc = ray.origin - sphere.center;
    float a = pow(length(ray.direction), 2);
    float half_b = dot(oc, ray.direction);
    float c = pow(length(oc), 2) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0) {
        return null_record();
    }
    float sqrtd = sqrt(discriminant);
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || root > t_max) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || root > t_max) {
            return null_record();
        }
    }
    vec3 p = ray_at(ray, root);
    vec3 outward_normal = (p - sphere.center) / sphere.radius;
    HitRecord rec = HitRecord(true, p, outward_normal, root, false, sphere.material);
    hit_record_set_face_normal(rec, ray);
    return rec;
}

#define SPHERES 5
const Sphere world[SPHERES] = Sphere[SPHERES] (
    Sphere(vec3(0.0, 0.0, -1.0), 0.5, Material(MATERIAL_DIFFUSE, vec3(0.7, 0.3, 0.3), 0.0)),
    Sphere(vec3(-1.1, 0.0, -1.0), 0.5, Material(MATERIAL_DIELECTRIC, vec3(1.0), 1.5)),
    Sphere(vec3(1.1, 0.0, -1.0), 0.5, Material(MATERIAL_METAL, vec3(0.7, 0.6, 0.5), 0.0)),
    Sphere(vec3(0.0, 0.0, -2.1), 0.5, Material(MATERIAL_METAL, vec3(0.8, 0.8, 0.8), 0.5)),
    Sphere(vec3(0.0, -100.5, -1.0), 100, Material(MATERIAL_DIFFUSE, vec3(0.8, 0.8, 0.0), 0.0))
);

HitRecord hit_world(float t_min, float t_max, Ray ray) {
    HitRecord temp_rec;
    HitRecord rec;
    bool hit_anything = false;
    float closest_so_far = t_max;
    for (int i = 0; i < SPHERES; i++) {
        temp_rec = hit_sphere(world[i], t_min, closest_so_far, ray);
        if (temp_rec.did_hit) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }
    if (hit_anything) {
        return rec;
    } else {
        return null_record();
    }
}

vec3 sky_color(vec3 ray_direction) {
    vec3 unit_direction = normalize(ray_direction);
    float t = 0.5 * (unit_direction.y + 1.0);
    return vec3(1.0 - t) * vec3(1.0) + vec3(t) * vec3(0.5, 0.7, 1.0);
}

vec3 ray_color(Ray ray) {
    Ray target = ray;
    vec3 accumulator = vec3(1.0);
    for (int i = 0; i < MAX_BOUNCE_DEPTH; i++) {
        HitRecord rec = hit_world(0.001, inf, target);
        if (!rec.did_hit) {
            accumulator *= sky_color(target.direction);
            break;
        }
        ScatterResult scattered;
        switch (rec.material.type) {
            case MATERIAL_DIFFUSE:
                scattered = material_diffuse_scatter(rec);
                break;
            case MATERIAL_METAL:
                scattered = material_metal_scatter(target, rec);
                break;
            case MATERIAL_DIELECTRIC:
                scattered = material_dielectric_scatter(target, rec);
                break;
        }
        if (scattered.did_scatter) {
            accumulator *= scattered.attenuation;
        } else {
            accumulator = vec3(0.0);
            break;
        }
        target = scattered.scattered;
    }
    return accumulator;
}

void main() {
    float aspect_ratio = u_resolution.x / u_resolution.y;
    float viewport_height = 2.0;
    float viewport_width = viewport_height * aspect_ratio;

    vec3 origin = u_camera_location;
    vec3 w = normalize(origin - u_camera_lookat);
    vec3 u = normalize(cross(u_camera_vup, w));
    vec3 v = cross(w, u);
    vec3 horizontal = viewport_width * u;
    vec3 vertical = viewport_height * v;
    vec3 lower_left_corner = origin - horizontal/2 - vertical/2 - w;
    
    vec3 color = vec3(0.0);
    for (int i = 0; i < SAMPLES; i++) {
        vec2 st = (gl_FragCoord.xy + vec2(rand(), rand())) / u_resolution;
        Ray r = Ray(origin, lower_left_corner + st.x * horizontal + st.y * vertical - origin);
        color += ray_color(r);
    }
    gl_FragColor = vec4(sqrt(color / vec3(SAMPLES)), 1.0f);
}