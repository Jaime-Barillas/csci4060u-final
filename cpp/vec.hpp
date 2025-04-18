#ifndef VEC_H
#define VEC_H

struct Vec {
  float x;
  float y;

  Vec operator-(const Vec &rhs);
  Vec operator*(float scalar) const;
  Vec operator/(float scalar) const;
  void operator=(const Vec &rhs);
  void operator+=(const Vec &rhs);
  void operator-=(const Vec &rhs);
  void operator/=(float scalar);

  float length() const;
  float length_squared() const;
  Vec normalized() const;
};

#endif
