#include <png++/png.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <vector>

using namespace std;

typedef struct Pt {
  int x, y, s;
} Pt;

int w = 1360, h = 768;
int ld = 5;
vector<int> xss = {0, 1, 1, 1, 0, -1, -1, -1};
vector<int> yss = {-1, -1, 0, 1, 1, 1, 0, -1};

int ColorDiff(png::rgb_pixel p1, png::rgb_pixel p2) {
  return abs(p1.red - p2.red) + abs(p1.green - p2.green) + abs(p1.blue - p1.blue);
}

void Edgify(png::image<png::rgb_pixel>& image) {
  int size = 3;
  for (size_t y = 0; y < image.get_height(); ++y) {
    for (size_t x = 0; x < image.get_width(); ++x) {
      int i = -size+1;
      loop:
      for (; i < size; i++) {
        for (int j = -size+1; j < size; j++) {
          if (i >= 0 && i < w && j >= 0 && j < h) {
            auto p1 = image[y][x];
            auto p2 = image[j][i];
            int diff = ColorDiff(p1, p2);
            if (diff > 200) {
              i = size;
              image[y][x] = png::rgb_pixel(255, 255, 255);
              image[y][x] = image[j][i];
              goto loop;
            }
          }
        }
      }
    }
  }
}

void Negapix(png::rgb_pixel& pix) {
  pix.red = 255 - pix.red;
  pix.green = 255 - pix.green;
  pix.blue = 255 - pix.blue;
}

void Desaturate(png::rgb_pixel& pix) {
  float f = 0.2;
  float l = 0.3 * pix.red + 0.6 * pix.green + 0.1 * pix.blue;
  pix.red += f * (l - pix.red);
  pix.green += f * (l - pix.green);
  pix.blue += f * (l - pix.blue);
}

void OffsetUp(png::rgb_pixel& pix, int m) {
  if (pix.red < 255 - m) pix.red += m;
  if (pix.green < 255 - m) pix.green += m;
  if (pix.blue < 255 - m) pix.blue += m;
}

void OffsetDown(png::rgb_pixel& pix, int m) {
  if (pix.red > m) pix.red -= m;
  if (pix.green > m) pix.green -= m;
  if (pix.blue > m) pix.blue -= m;
}

void SetPixel(int x, int y, png::image<png::rgb_pixel>& image, float m) {
  if (m < 0) return;
  if (x < 0 || x >= w || y < 0 || y >= h) return;
  auto pix = image[y][x];
  // Desaturate(pix);
  OffsetUp(pix, round(m));
  image[y][x] = png::rgb_pixel(pix.red, pix.green, pix.blue);
}

void SetPixel2(int x, int y, png::image<png::rgb_pixel>& image, float m) {
  if (m < 0) return;
  if (x < 0 || x >= w || y < 0 || y >= h) return;
  auto pix = image[y][x];
  OffsetDown(pix, round(m));
  image[y][x] = png::rgb_pixel(pix.red, pix.green, pix.blue);
}

void Line(float x1, float y1, float x2, float y2,
          png::rgb_pixel pix, png::image<png::rgb_pixel>& image) {
  const bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
  if (steep) {
    std::swap(x1, y1);
    std::swap(x2, y2);
  }

  if (x1 > x2) {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }

  const float dx = x2 - x1;
  const float dy = fabs(y2 - y1);

  float error = dx / 2.0f;
  const int ystep = (y1 < y2) ? 1 : -1;
  int y = (int)y1;

  const int maxX = (int)x2;

  float mmax = 5;
  float m = 0;
  float adder = -0.03;
  for (int x = (int)x1; x < maxX; x++) {
    if (steep) {
      SetPixel(y,x, image, m);
    } else {
      SetPixel(x,y, image, m);
    }

    m -= adder;
    if (m < 0 || m > mmax) {
      adder = -adder;
    }

    error -= dy;
    if (error < 0) {
      y += ystep;
      error += dx;
    }
  }
}

void CrossMul(png::image<png::rgb_pixel>& image) {
  for (size_t y = 0; y < image.get_height(); ++y) {
    for (size_t x = 0; x < image.get_width(); ++x) {
      image[y][x].red = image[y][x].blue * image[y][x].green;
    }
  }
}

void Circle(int x, int y, int r, png::image<png::rgb_pixel>& image) {
  int xi = r;
  int yi = 0;
  int decision_over = 1 - r;

  float m = 5;
  while (yi <= xi) {
    SetPixel2( xi + x,  yi + y, image, m);
    SetPixel2( yi + x,  xi + y, image, m);
    SetPixel2(-xi + x,  yi + y, image, m);
    SetPixel2(-yi + x,  xi + y, image, m);
    SetPixel2(-xi + x, -yi + y, image, m);
    SetPixel2(-yi + x, -xi + y, image, m);
    SetPixel2( xi + x, -yi + y, image, m);
    SetPixel2( yi + x, -xi + y, image, m);
    yi++;
    if (decision_over <= 0) {
      decision_over += 2 * yi + 1;
    } else {
      xi--;
      decision_over += 2 * (yi - xi) + 1;
    }
  }
}

void Glass(float n, int fac, png::image<png::rgb_pixel>& image) {
  for (int i = 0; i != w*h*n; i++) {
    int x = rand() % w;
    int y = rand() % h;
    int x2 = x + ((rand() % fac*2) - fac);
    int y2 = y + ((rand() % fac*2) - fac);
    if (y2 >= 0 && y2 < h && x2 >= 0 && x2 < w) {
      png::rgb_pixel p(image[y][x].red, image[y][x].green, image[y][x].blue);
      image[y][x].red = image[y2][x2].red;
      image[y][x].green = image[y2][x2].green;
      image[y][x].blue = image[y2][x2].blue;
      image[y2][x2].red = p.red;
      image[y2][x2].green = p.green;
      image[y2][x2].blue = p.blue;
    }
  }
}

void PicIntl(string path, vector<Pt> ps) {
  float rmul, gmul, bmul;
  cout << ps.size() << endl;
  switch (rand() % 3) {
    case 0:
    rmul = (rand() % 8 + 5) / 10.0f;
    gmul = rmul + (rand() % 7 - 3) / 10.0f;
    bmul = rmul + (rand() % 7 - 3) / 10.0f;
    break;
    case 1:
    gmul = (rand() % 8 + 5) / 10.0f;
    rmul = gmul + (rand() % 7 - 3) / 10.0f;
    bmul = gmul + (rand() % 7 - 3) / 10.0f;
    break;
    case 2:
    bmul = (rand() % 8 + 5) / 10.0f;
    rmul = bmul + (rand() % 7 - 3) / 10.0f;
    gmul = bmul + (rand() % 7 - 3) / 10.0f;
    break;
  }

  png::image<png::rgb_pixel> image(w, h);
  for (size_t y = 0; y < image.get_height(); ++y) {
    for (size_t x = 0; x < image.get_width(); ++x) {
      int r = 0, g = 0, b = 0, d = 0;
      for (auto p : ps) {
        d = sqrt((x - p.x)*(x - p.x) + (y - p.y)*(y - p.y)) / 20;
        float q = (p.s / (d+1));
        // if (d > p.s) {
        r += q * rmul;
        g += q * gmul;
        b += q * bmul;
        // }
        image[y][x] = png::rgb_pixel(r, g, b);
        // gmul += 0.000001;
        // bmul += 0.000001;
      }
    }
  }

  png::rgb_pixel pix(255, 255, 0);
  for (auto p : ps) {
    int cnt = 0;
    for (auto p2 : ps) {
      if (cnt++ > 4) continue;
      Line((float)p.x, (float)p.y, (float)p2.x, (float)p2.y, pix, image);
      Line((float)p.x+1, (float)p.y, (float)p2.x, (float)p2.y, pix, image);
      Line((float)p.x+1, (float)p.y-1, (float)p2.x, (float)p2.y, pix, image);
    }
  }

  // int circs = 2;
  // bool breaker = false;
  // for (Pt p1 : ps) {
  //   for (Pt p2 : ps) {
  //     float dist = sqrt((p2.x - p1.x)*(p2.x - p1.x) + (p2.y - p1.y)*(p2.y - p1.y));
  //     if (dist > 10) {
  //       for (int i = -5; i != 6; i++) {
  //         Circle((p2.x + p1.x) / 2, (p2.y + p1.y) / 2, dist / 2 + i, image);
  //       }
  //     }
  //     circs--;
  //     if (circs == 0) {
  //       breaker = true;
  //       break;
  //     }
  //   }
  //   if (breaker) break;
  // }

  // Glass(10, 1, image);

  image.write(path);
}

void Pic(string path, int acnt) {
  int cnt = acnt;
  int dist = 2000;
  int dist_r = 500;
  int true_dist = dist - rand() % dist_r;
  int divv = 20;
  vector<Pt> ps;
  int margin = 200;
  for (int i = 0; i < cnt; i++) {
    bool go = true;

    if (i == 2) {

    }

    while (go) {
      int x = rand() % (w-margin*2) + margin;
      int y = rand() % (h-margin*2) + margin;
      bool close = false;
      bool far = true;
      for (Pt p : ps) {
        float dist = sqrt((x - p.x)*(x - p.x) + (y - p.y)*(y - p.y));
        if (dist < 400 && dist > 200) {
          close = true;
        }
        if (dist < 200) {
          far = false;
        }
      }
      if ((close && far) || ps.size() == 0) {
        go = false;
        ps.push_back({x, y, true_dist});
      }
    }
  }
  PicIntl(path, ps);
}


int main(int argc, char** argv) {
  cout << argc << endl;

  string path = "/home/owen/Pictures/background.png";
  srand(time(NULL));

  vector<Pt> ps;
  if (argc > 1) {
    for (int i = 1; i < argc; i += 3) {
      ps.push_back({atoi(argv[i]), atoi(argv[i+1]), atoi(argv[i+2])});
    }
    PicIntl(path, ps);
  } else {
    int r = rand() % 1000;
    if (r < 10) {
      Pic("/home/owen/Pictures/background.png", 6);
    } else if (r < 30) {
      Pic("/home/owen/Pictures/background.png", 5);
    } else if (r < 80) {
      Pic("/home/owen/Pictures/background.png", 4);
    } else if (r < 250) {
      Pic("/home/owen/Pictures/background.png", 1);
    } else if (r < 500) {
      Pic("/home/owen/Pictures/background.png", 3);
    } else {
      Pic("/home/owen/Pictures/background.png", 2);
    }
  }
}
