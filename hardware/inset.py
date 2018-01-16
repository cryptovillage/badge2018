#!/usr/bin/python

# (gr_line (start 212.356666 127.998484) (end 196.768185 136.998483) (layer Edge.Cuts) (width 0.4))

import re
import fileinput
import math

lines = []
points = []
pattern = re.compile('\s*\(gr_line \(start ([-+]?(\d+(\.\d*)?|\.\d+)) ([-+]?(\d+(\.\d*)?|\.\d+))\) \(end ([-+]?(\d+(\.\d*)?|\.\d+)) ([-+]?(\d+(\.\d*)?|\.\d+))\) \(layer Cmts')

for line in fileinput.input():
  m = pattern.match(line)
  lines.append(((float(m.group(1)), float(m.group(4))), (float(m.group(7)), float(m.group(10)))))
  points.append((float(m.group(7)), float(m.group(10))))

#for i in range(len(lines)):
#  print(lines[i])
#  for j in range(len(lines)):
#    if abs(lines[i][1][0] - lines[j][0][0]) < 0.01 and abs(lines[i][1][1] - lines[j][0][1]) < 0.01:
#      print("%s -> %s" % (i, j))

def lineIntersection(Ax, Ay, Bx, By, Cx, Cy, Dx, Dy):
  # Fail if either line is undefined.
  if (Ax == Bx and Ay == By) or (Cx == Dx and Cy == Dy):
    return None

  # (1) Translate the system so that point A is on the origin.
  Bx -= Ax
  By -= Ay
  Cx -= Ax
  Cy -= Ay
  Dx -= Ax
  Dy -= Ay

  # Discover the length of segment A-B.
  distAB = math.sqrt(Bx*Bx+By*By)

  # (2) Rotate the system so that point B is on the positive X axis.
  theCos = Bx / distAB
  theSin = By / distAB
  newX = Cx * theCos + Cy * theSin
  Cy = Cy * theCos - Cx * theSin
  Cx = newX
  newX = Dx * theCos + Dy * theSin
  Dy = Dy * theCos - Dx * theSin
  Dx = newX

  # Fail if the lines are parallel.
  if Cy == Dy:
    return None;

  # (3) Discover the position of the intersection point along line A-B.
  ABpos = Dx + (Cx - Dx) * Dy / (Dy - Cy);

  # (4) Apply the discovered position to line A-B in the original coordinate system.
  return (Ax + ABpos * theCos, Ay + ABpos * theSin)

def insetCorner(a, b, c, d, e, f, insetDist):
  c1 = c
  d1 = d
  c2 = c
  d2 = d

  dx1 = c - a
  dy1 = d - b
  dist1 = math.sqrt(dx1*dx1+dy1*dy1);
  dx2 = e - c
  dy2 = f - d
  dist2 = math.sqrt(dx2*dx2+dy2*dy2);

  if dist1 == 0.0 or dist2 == 0:
    #raise ValueError("Encountered zero-length segment")
    return None

  insetX =  dy1/dist1*insetDist
  a += insetX
  c1 += insetX
  insetY = -dx1/dist1*insetDist;
  b += insetY
  d1 += insetY
  insetX = dy2/dist2*insetDist
  e += insetX
  c2 += insetX
  insetY = -dx2/dist2*insetDist
  f += insetY
  d2+=insetY

  if c1 == c2 and d1 == d2:
    return (c1, d1)

  return lineIntersection(a,b,c1,d1,c2,d2,e,f)

def insetPolygon(points, insetDist):
  insetPoints = []
  startX = points[0][0]
  startY = points[0][1]
  corners = len(points)

  if corners < 3:
    raise ValueError("Polygon must have at least three corners")

  # Inset the polygon.
  c = points[corners-1][0]
  d = points[corners-1][1]
  e = points[0][0]
  f = points[0][1]

  for i in range(corners - 1):
    a = c
    b = d
    c = e
    d = f
    e = points[i+1][0]
    f = points[i+1][1];
    insetPoints.append(insetCorner(a,b,c,d,e,f,insetDist))
  insetPoints.append(insetCorner(c,d,e,f,startX,startY,insetDist))
  return insetPoints

# (gr_line (start 212.356666 127.998484) (end 196.768185 136.998483) (layer Edge.Cuts) (width 0.4))

insetPoints = insetPolygon(points, -0.2)

for i in range(len(insetPoints) - 1):
  print("  (gr_line (start %s %s) (end %s %s) (layer Edge.Cuts) (width 0.4))" %
    (insetPoints[i][0], insetPoints[i][1], insetPoints[i + 1][0], insetPoints[i + 1][1]))
print("  (gr_line (start %s %s) (end %s %s) (layer Edge.Cuts) (width 0.4))" %
    (insetPoints[-1][0], insetPoints[-1][1], insetPoints[0][0], insetPoints[0][1]))

