#ifndef __PLUGIN_BPALIGN_UTILS_REG_H__
#define __PLUGIN_BPALIGN_UTILS_REG_H__

template <typename T>
class RegEnable {
private:
    T Dout;
public:
    T Din;
    bool en;
    int tick() { if (en) Dout = Din; }
    T read() {return Dout;}
};



#endif