// SPDX-License-Identifier: 0BSD
//
// arczi84's nfs3_fpu_bench.cpp (source.sha256 is its hash as received), with
// its main() replaced by the two entry points benches/nfs3.c calls; the
// kernels and the input generators are unchanged.

/* nfs3_fpu_bench.cpp: two floating-point kernels of the NFS3 68k port, as
 * the race runs them, in one file for compiler work (no game, no OS calls
 * beyond printf and clock).
 *
 *   project  projectObjectVertex<GuardedFloatMath>: a model vertex through a
 *            3x3 matrix and translation, then the perspective divide; every
 *            result checked for overflow/underflow (include/port/object_geometry.h)
 *   clip     PolygonClipWork::run<GuardedFloatMath>: a triangle clipped
 *            against the near plane and the four screen edges, nine floats
 *            interpolated per new vertex (include/port/polygon_clip.h)
 *
 * Both are among the hottest native functions in the race profile on a
 * PiStorm (68040 code under Emu68).  The kernels are copied from the port
 * unchanged; only the driver below is new.  Inputs come from a fixed LCG, so
 * the checksums must not change between compilers or flags: a different
 * checksum is a miscompile (or a change in float rounding).
 *
 * Build:  m68k-amigaos-gcc -O2 -m68040 -mhard-float -noixemul -fno-exceptions \
 *             -o nfs3_fpu_bench nfs3_fpu_bench.cpp
 * Run:    nfs3_fpu_bench [rounds]      (default 20; each round is 100000 vertices
 *                                       and 20000 triangles)
 *
 * The FPU runs in single rounding precision (FPCR 0x40, as in the race).
 * Reference, GCC 16.2 rc15 -O2 -m68040, PiStorm (Emu68):
 *   5 rounds:   project 7a596329  clip 77bb4ca6   (0.42 s, 0.48 s)
 *   50 rounds:  project d15a354b  clip 7d8394d6   (4.20 s, 4.98 s)
 * A PC (g++ -O2, x86-64 SSE) gives other sums (5 rounds: 449f5330, 4352ff86):
 * the 68k code does not round every intermediate the same way.  Compare
 * 68k builds with each other; a change of sum means the float results
 * changed (a miscompile, or a reassociation the flags allow).
 */
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <initializer_list>

#include "nfs3.h"

/* ---- include/port/guarded_float_math.h ---- */
namespace nfsport {
// A checked float loop may be replayed with the exact x87 arithmetic when a
// result leaves the normal float range. No host exception flags are required.
struct GuardedFloatMath {
    using Value=float;
    bool* fallback;
    explicit GuardedFloatMath(bool* failure=nullptr):fallback(failure){}
    static uint32_t bits(Value value){uint32_t result;std::memcpy(&result,&value,4);return result;}
    Value check(Value result,Value a=0,Value b=0,bool product=false) const {
        if(fallback){
            const uint32_t magnitude=bits(result)&0x7fffffff;
            const uint32_t exponent=magnitude&0x7f800000;
            if(exponent==0x7f800000 || (!exponent &&
                (magnitude || (product && (bits(a)&0x7fffffff) && (bits(b)&0x7fffffff)))))
                *fallback=true;
        }
        return result;
    }
    Value add(Value a,Value b) const {return check(a+b);}
    Value sub(Value a,Value b) const {return check(a-b);}
    Value mul(Value a,Value b) const {return check(a*b,a,b,true);}
    Value div(Value a,Value b) const {return check(a/b,a,b,true);}
    Value sqrt(Value a) const {return check(std::sqrt(a));}
};
}

/* ---- include/port/object_geometry.h (the portable part) ---- */
namespace nfsport {
struct ProjectedObjectVertex {float x,y,z,inverse;};
inline uint32_t objectFloatBits(float value){uint32_t bits;std::memcpy(&bits,&value,4);return bits;}
// Native arithmetic only: no guest registers, stack or memory adapters.
// Float temporaries preserve the original stores between transform and projection.
template<class Math>
ProjectedObjectVertex projectObjectVertex(float vx,float vy,float vz,const float* m,const float* t,
                                         float sx,float sy,float cx,float cy,const Math& math){
    const auto rx=math.add(math.add(math.mul(vx,m[0]),math.mul(vy,m[3])),math.mul(vz,m[6]));
    const auto ry=math.add(math.add(math.mul(vx,m[1]),math.mul(vy,m[4])),math.mul(vz,m[7]));
    const auto rz=math.add(math.add(math.mul(vx,m[2]),math.mul(vy,m[5])),math.mul(vz,m[8]));
    const float x=float(math.add(rx,t[0])),y=float(math.add(ry,t[1]));
    float z=float(math.add(rz,t[2]));
    if(objectFloatBits(z)==0x3f800000u){const uint32_t tiny=0x37800080u;std::memcpy(&z,&tiny,4);}
    const float inverse=float(math.div(1.0f,z));
    const auto overX=math.mul(sx,inverse),overY=math.mul(sy,inverse);
    const auto projectedY=math.mul(overY,y),projectedX=math.mul(overX,x);
    return {float(math.add(projectedX,cx)),float(math.add(projectedY,cy)),z,inverse};
}
}

/* ---- include/port/polygon_clip.h ---- */
namespace nfsport {
// Host-owned clipping records. No CPU, guest stack, endian proxy or driver
// call is accessed inside the polygon passes. The adapter commits the work
// records once and handles the legacy call boundary.
struct PolygonClipWork {
    static constexpr unsigned capacity=64, listCapacity=128;
    struct Vertex {float field[9];};
    Vertex vertices[capacity];
    unsigned lists[listCapacity];
    uint8_t codes[listCapacity], initialCodes[3];
    unsigned count=3,made=0,listStart=0,outStart=0,written=0;
    unsigned codeStart=0,outCodes=0,codeWritten=0;
    bool initialList=true,initialCodeList=true,compared=false,rejected=false;
    unsigned rejectedCount=0;
    double comparedA=0,comparedB=0;
    float nearW,top,bottom,left,right;
    uint8_t clipCodes;
    // If the incoming code bytes alias the output code storage, preserve
    // the original read-after-write behavior instead of copying them aside.
    int inputCodeOffset=-1;
    static uint32_t bits(float f){uint32_t b;std::memcpy(&b,&f,4);return b;}
    static uint32_t address(unsigned i){return i<3?0x7d3570u+36*i:0x7d1198u+36*(i-3);}
    void compare(double a,double b){compared=true;comparedA=a;comparedB=b;}
    unsigned index(unsigned i)const{return initialList?i:lists[listStart+i];}
    uint8_t code(unsigned i)const{
        if(!initialCodeList)return codes[codeStart+i];
        return inputCodeOffset>=0?codes[unsigned(inputCodeOffset)+i]:initialCodes[i];
    }
    template<class M> void between(const M& math,unsigned a,unsigned b,unsigned vertex,unsigned f,typename M::Value t){
        const typename M::Value from=vertices[a].field[f];
        vertices[vertex].field[f]=float(math.sub(from,math.mul(math.sub(from,vertices[b].field[f]),t)));
    }
    void codeX(unsigned v,uint8_t& c){
        const double x=vertices[v].field[0];compare(x,left);
        if(!(x>=left))c|=1;else{compare(x,right);if(x>right)c|=2;}
    }
    template<class M> void intersection(const M& math,uint8_t bit,unsigned a,unsigned b,unsigned v,uint8_t& c,uint8_t previous){
        if(bit==0x10){
            const typename M::Value aW=vertices[a].field[2];
            const typename M::Value inverse=float(math.div(1.0,math.sub(aW,vertices[b].field[2])));
            typename M::Value t=float(math.mul(math.sub(aW,nearW),inverse));
            for(unsigned f:{0u,1u,3u,4u})between<M>(math,a,b,v,f,t);
            if(int32_t(bits(float(t)))>int32_t(0x3f800000))t=float(math.mul(aW,inverse));
            else{compare(0.0,t);if(0.0>t)t=float(math.mul(aW,inverse));}
            for(unsigned f:{5u,6u,7u,8u})between<M>(math,a,b,v,f,t);
            vertices[v].field[2]=nearW;c=0x10;
            const double y=vertices[v].field[1];compare(y,bottom);
            if(!(y>=bottom))c|=8;else{compare(y,top);if(y>top)c|=4;}
            codeX(v,c);
        }else{
            const unsigned axis=(bit&0xc)?1:0;
            const float edge=bit==4?top:bit==8?bottom:bit==1?left:right;
            const typename M::Value from=vertices[a].field[axis];
            const typename M::Value t=math.div(math.sub(from,edge),math.sub(from,vertices[b].field[axis]));
            for(unsigned f=0;f<9;++f)if(f!=axis)between<M>(math,a,b,v,f,t);
            vertices[v].field[axis]=edge;
            c=uint8_t((previous&(axis?0xf0:0xfc))|bit);
            if(axis)codeX(v,c);
        }
    }
    // false means bounded local storage could not represent this input;
    // the adapter replays the legacy path before committing any writes.
    template<class M> bool run(const M& math){
        for(uint8_t bit:{uint8_t(0x10),uint8_t(4),uint8_t(8),uint8_t(1),uint8_t(2)}){
            if(!(clipCodes&bit))continue;
            unsigned kept=0,previous=count-1;
            for(unsigned current=0;current<count;previous=current++){
                const uint8_t previousCode=code(previous);
                if(!(previousCode&bit)){
                    if(outStart+kept>=listCapacity || outCodes+kept>=listCapacity)return false;
                    lists[outStart+kept]=index(previous);codes[outCodes+kept]=previousCode;++kept;
                }
                if((previousCode^code(current))&bit){
                    if(made+3>=capacity || outStart+kept>=listCapacity || outCodes+kept>=listCapacity)return false;
                    const unsigned v=3+made;
                    intersection<M>(math,bit,index(previous),index(current),v,codes[outCodes+kept],previousCode);
                    lists[outStart+kept]=v;++made;++kept;
                }
            }
            written=outStart+kept;codeWritten=outCodes+kept;
            if(kept<3){rejected=true;rejectedCount=kept;return true;}
            initialList=initialCodeList=false;listStart=outStart;codeStart=outCodes;
            outStart+=kept;outCodes+=kept;count=kept;
            // The original recomputes after near and after both Y planes.
            if(bit==0x10 || bit==8 || (bit==4 && !(clipCodes&8))){
                clipCodes=0;for(unsigned i=0;i<count;++i)clipCodes|=code(i);
            }
        }
        return true;
    }
    void resetPasses(uint8_t originalClipCodes){
        count=3;made=listStart=outStart=written=codeStart=outCodes=codeWritten=0;
        initialList=initialCodeList=true;compared=rejected=false;rejectedCount=0;clipCodes=originalClipCodes;
        if(inputCodeOffset>=0)for(unsigned i=0;i<3;++i)codes[unsigned(inputCodeOffset)+i]=initialCodes[i];
    }
    uint32_t listAddress()const{return initialList?0x5600d4u:0x7d0e78u+4*listStart;}
};
}

/* ---- driver ---- */
static uint32_t seed = 0x12345678u;
static float uniform(float low, float high)
{
    seed = seed * 1664525u + 1013904223u;
    return low + (high - low) * float(seed >> 8) * (1.0f / 16777216.0f);
}
static uint32_t bitsOf(float f) { uint32_t b; std::memcpy(&b, &f, 4); return b; }
static uint32_t mix(uint32_t sum, float f) { return (sum ^ bitsOf(f)) * 16777619u; }

static uint32_t project(unsigned count)
{
    using namespace nfsport;
    float m[9], t[3];
    for (float& v : m) v = uniform(-1.0f, 1.0f);
    t[0] = uniform(-50.0f, 50.0f); t[1] = uniform(-20.0f, 20.0f); t[2] = uniform(20.0f, 200.0f);
    bool fallback = false;
    const GuardedFloatMath math(&fallback);
    uint32_t sum = 2166136261u;
    for (unsigned i = 0; i < count; ++i)
    {
        const float x = uniform(-10.0f, 10.0f), y = uniform(-10.0f, 10.0f), z = uniform(-10.0f, 10.0f);
        const ProjectedObjectVertex p = projectObjectVertex(x, y, z, m, t, 320.0f, 240.0f, 320.0f, 240.0f, math);
        sum = mix(mix(mix(mix(sum, p.x), p.y), p.z), p.inverse);
    }
    return sum ^ unsigned(fallback);
}

static uint8_t outcode(const float* v, const nfsport::PolygonClipWork& w)
{
    uint8_t c = 0;
    if (!(v[2] >= w.nearW)) c |= 0x10;
    if (v[1] < w.bottom) c |= 8; else if (v[1] > w.top) c |= 4;
    if (v[0] < w.left) c |= 1; else if (v[0] > w.right) c |= 2;
    return c;
}

static uint32_t clip(unsigned count)
{
    using namespace nfsport;
    static PolygonClipWork work;  // 2.8 KB: not on a small Amiga stack
    bool fallback = false;
    const GuardedFloatMath math(&fallback);
    uint32_t sum = 2166136261u;
    unsigned made = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        work.nearW = 1.0f; work.top = 479.0f; work.bottom = 0.0f; work.left = 0.0f; work.right = 639.0f;
        work.inputCodeOffset = -1;
        uint8_t all = 0;
        for (unsigned k = 0; k < 3; ++k)
        {
            float* v = work.vertices[k].field;
            v[0] = uniform(-200.0f, 840.0f); v[1] = uniform(-150.0f, 630.0f); v[2] = uniform(0.2f, 50.0f);
            for (unsigned f = 3; f < 9; ++f) v[f] = uniform(0.0f, 256.0f);
            work.initialCodes[k] = outcode(v, work);
            all |= work.initialCodes[k];
        }
        work.resetPasses(all);
        if (!work.run(math)) continue;
        made += work.made;
        for (unsigned k = 0; k < work.made; ++k)
            for (float f : work.vertices[3 + k].field) sum = mix(sum, f);
        sum = (sum ^ work.written ^ (unsigned(work.rejected) << 8)) * 16777619u;
    }
    return sum ^ made ^ unsigned(fallback);
}

/* ---- benchwork entry points ---- */

// Single rounding precision, as the race runs (the port sets the same);
// without it GCC's FPU code keeps extended intermediates. Returns the FPCR
// to put back, so the other benchmarks keep the default.
static uint32_t fpcr_single()
{
    uint32_t old = 0;
#if defined(__mc68000__) && defined(__HAVE_68881__)
    __asm__ volatile("fmove%.l %%fpcr,%0" : "=d"(old));
    __asm__ volatile("fmove%.l %0,%%fpcr" : : "d"(0x40));
#endif
    return old;
}

static void fpcr_restore(uint32_t old)
{
#if defined(__mc68000__) && defined(__HAVE_68881__)
    __asm__ volatile("fmove%.l %0,%%fpcr" : : "d"(old));
#endif
    (void)old;
}

// ROUNDS rounds of COUNT vertices, from the seed the original main()
// started with, so that 5 rounds of 100000 give its reference sum.
extern "C" uint32_t nfs3_project(unsigned rounds, unsigned count)
{
    const uint32_t fpcr = fpcr_single();
    uint32_t p = 0;
    seed = 0x12345678u;
    for (unsigned r = 0; r < rounds; ++r) p ^= project(count) + r;
    fpcr_restore(fpcr);
    return p;
}

// ROUNDS rounds of COUNT triangles, from the original's second seed, so
// that 5 rounds of 20000 give its reference sum.
extern "C" uint32_t nfs3_clip(unsigned rounds, unsigned count)
{
    const uint32_t fpcr = fpcr_single();
    uint32_t c = 0;
    seed = 0x9e3779b9u;
    for (unsigned r = 0; r < rounds; ++r) c ^= clip(count) + r;
    fpcr_restore(fpcr);
    return c;
}
