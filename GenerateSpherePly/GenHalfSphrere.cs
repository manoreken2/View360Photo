// 日本語。
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace GenerateSpherePly {

    class GenHalfSphere {
        private int mXCount;
        private int mYCount;
        private List<Vertex> mVtxList = new List<Vertex>();
        private List<int> mTriIdxList = new List<int>();

        public void Run(string path, int xCount, int yCount, int n) {
            mXCount = xCount;
            mYCount = yCount;

            CreateHalfSphere(n);

            using (var tw = File.CreateText(path)) {
                WriteHeader(tw);
                WriteContent(tw);
            }
        }

        /// <summary>
        /// UV値を0～1の範囲にする。
        /// </summary>
        static double WrapUV(double uv) {
            // 誤差により、若干0よりも小さい値になることがあるが
            // その場合はそのままにする。
            while (uv < -1e-8) {
                uv += 1.0;
            }

            return uv;
        }

        // https://en.wikipedia.org/wiki/Equirectangular_projection
        void CreateHalfSphere(int n) {
            mVtxList.Clear();
            mTriIdxList.Clear();

            // https://en.wikipedia.org/wiki/Sphere

            for (int x = 0; x <= mXCount; ++x) {
                // φ: 経度 (0°～ 180°)
                // n==0のとき、φ = 0 → π  : s = 1.0 → 0
                // n==1のとき、φ = π → 2π : s = 1.0 → 0
                double φf = 1.0 * Math.PI * x / mXCount;
                double φ = φf + n * Math.PI;

                for (int y = 0; y <= mYCount; ++y) {
                    // θ: 緯度 +1 → -1 (0°～ 180°)
                    double θ = 1.0 * Math.PI * y / mYCount;

                    // テクスチャーUV値は、0～1
                    // OpenGLのとき  (0,0)が左下、(1,1)が右上。
                    // DirectXのとき (0,0)が左上、(1,1)が右下。
                    double s = WrapUV(1.0 - φf / (1.0 * Math.PI));
                    double t = WrapUV(1.0 - θ  / (1.0 * Math.PI));

                    // 頂点を作成します。
                    var vtx = new Vertex();
                    vtx.x = Math.Sin(θ) * Math.Cos(φ);
                    vtx.y = Math.Cos(θ);
                    vtx.z = Math.Sin(θ) * Math.Sin(φ);
                    vtx.s = s;
                    vtx.t = t;
                    AddVertex(vtx);
                }
            }

            for (int x = 0; x < mXCount; ++x) {
                // φ: 経度 (0°～ 180°)
                // n==0のとき、φ = 0 → π  : s = 1.0 → 0
                // n==1のとき、φ = π → 2π : s = 1.0 → 0
                double φ0f = 1.0 * Math.PI * (x + 0) / mXCount;
                double φ1f = 1.0 * Math.PI * (x + 1) / mXCount;
                double φ0 = φ0f + n * Math.PI;
                double φ1 = φ1f + n * Math.PI;
                double s0 = WrapUV(1.0 - φ0f / (1.0 * Math.PI));
                double s1 = WrapUV(1.0 - φ1f / (1.0 * Math.PI));

                for (int y = 0; y < mYCount; ++y) {
                    // θ: 緯度 +1 → -1 (0°～ 180°)
                    double θ0 = 1.0 * Math.PI * (y + 0) / mYCount;
                    double θ1 = 1.0 * Math.PI * (y + 1) / mYCount;

                    double t0 = WrapUV(1.0 - θ0 / (1.0 * Math.PI));
                    double t1 = WrapUV(1.0 - θ1 / (1.0 * Math.PI));

                    int idx00 = FindVertex(
                        Math.Sin(θ0) * Math.Cos(φ0),
                        Math.Cos(θ0),
                        Math.Sin(θ0) * Math.Sin(φ0),
                        s0, t0);
                    int idx10 = FindVertex(
                        Math.Sin(θ1) * Math.Cos(φ0),
                        Math.Cos(θ1),
                        Math.Sin(θ1) * Math.Sin(φ0),
                        s0, t1);
                    int idx01 = FindVertex(
                        Math.Sin(θ0) * Math.Cos(φ1),
                        Math.Cos(θ0),
                        Math.Sin(θ0) * Math.Sin(φ1),
                        s1, t0);
                    int idx11 = FindVertex(
                        Math.Sin(θ1) * Math.Cos(φ1),
                        Math.Cos(θ1),
                        Math.Sin(θ1) * Math.Sin(φ1),
                        s1, t1);

                    // counter clock wiseで内側にメッシュを貼る。
                    AddTriangle(idx00, idx10, idx01);
                    AddTriangle(idx11, idx01, idx10);
                }
            }
        }

        void AddTriangle(int a, int b, int c) {
            if (a == b || a == c || b == c) {
                return;
            }

            for (int i = 0; i < mTriIdxList.Count / 3; ++i) {
                int tA = mTriIdxList[i * 3 + 0];
                int tB = mTriIdxList[i * 3 + 1];
                int tC = mTriIdxList[i * 3 + 2];
                if (tA == a && tB == b && tC == c) {
                    return;
                }
            }

            mTriIdxList.Add(a);
            mTriIdxList.Add(b);
            mTriIdxList.Add(c);
        }

        /// <summary>
        /// returns vertex index
        /// </summary>
        int AddVertex(Vertex v) {
            for (int i = 0; i < mVtxList.Count; ++i) {
                var a = mVtxList[i];
                if (a.IsSimilarTo(v)) {
                    return i;
                }
            }

            mVtxList.Add(v);
            return mVtxList.Count - 1;
        }

        int FindVertex(double x, double y, double z, double s, double t) {
            for (int i = 0; i < mVtxList.Count; ++i) {
                var a = mVtxList[i];
                if (a.IsSimilarTo(x, y, z, s, t)) {
                    return i;
                }
            }
            throw new ArgumentException("could not found");
        }

        void WriteHeader(TextWriter tw) {
            tw.WriteLine("ply");
            tw.WriteLine("format ascii 1.0");
            tw.WriteLine("element vertex {0}", mVtxList.Count);
            tw.WriteLine("property float x");
            tw.WriteLine("property float y");
            tw.WriteLine("property float z");
            tw.WriteLine("property float s");
            tw.WriteLine("property float t");
            tw.WriteLine("element face {0}", mTriIdxList.Count / 3);
            tw.WriteLine("property list uchar uint vertex_indices");
            tw.WriteLine("end_header");
        }

        void WriteContent(TextWriter tw) {
            foreach (var v in mVtxList) {
                tw.WriteLine("{0} {1} {2} {3} {4}", v.x, v.y, v.z, v.s, v.t);
            }
            for (int i = 0; i < mTriIdxList.Count / 3; ++i) {
                int idx0 = mTriIdxList[i * 3 + 0];
                int idx1 = mTriIdxList[i * 3 + 1];
                int idx2 = mTriIdxList[i * 3 + 2];
                tw.WriteLine("3 {0} {1} {2}", idx0, idx1, idx2);
            }
        }
    }
}
