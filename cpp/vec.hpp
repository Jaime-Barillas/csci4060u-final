#ifndef VEC_H
#define VEC_H

struct Vec {
  float x;
  float y;

  Vec operator-(const Vec &rhs);
  Vec operator*(float scalar) const;
  void operator+=(const Vec &rhs);

  float length() const;
};

#endif
