// 日本語。
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace GenerateSpherePly {

    class Program {
        static void Main(string[] args) {
#if true
            var instance = new GenHalfSphere();
            instance.Run("sphereL.ply", 64, 64, 0);
            instance.Run("sphereR.ply", 64, 64, 1);
#else
            var instance = new GenSphere();
            instance.Run("sphere.ply", 64, 32);
#endif
            Console.WriteLine("Done.");
        }
    }
}
