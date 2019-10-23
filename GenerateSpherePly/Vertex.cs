using System;
using System.Collections.Generic;
using System.Text;

namespace GenerateSpherePly {
    struct Vertex {
        public double x;
        public double y;
        public double z;
        public double s; //< texture U
        public double t; //< texture V

        public bool IsSimilarTo(Vertex rhs) {
            if (Math.Abs(x - rhs.x) + Math.Abs(y - rhs.y) + Math.Abs(z - rhs.z)
                + Math.Abs(s - rhs.s) + Math.Abs(t - rhs.t) < 1e-8) {
                return true;
            }
            return false;
        }

        public bool IsSimilarTo(double aX, double aY, double aZ, double aS, double aT) {
            if (Math.Abs(x - aX) + Math.Abs(y - aY) + Math.Abs(z - aZ)
                 + Math.Abs(s - aS) + Math.Abs(t - aT) < 1e-8) {
                return true;
            }
            return false;
        }
    };

}
