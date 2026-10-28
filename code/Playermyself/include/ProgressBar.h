#pragma once
#include<SDL2/SDL.h>

class ProgressBar{

public:
//构造和析沟
    ProgressBar();
    ~ProgressBar();

//禁止拷贝构造和深拷贝
ProgressBar(const ProgressBar&) = delete;
ProgressBar& operator = (const ProgressBar&) = delete;

public:
 //对外调用的接口
 
    //进度条参数
    SDL_Rect m_bgrect{};
    double m_windowHeight = 0;              //长宽
    double m_windowWidth = 0;

    SDL_Renderer* m_renderer = nullptr;

    //进度条，初始化，更新，画
    void progress_init(double windowwidth,double windowheight,SDL_Renderer* renderer);
    void updateprogress(double current_time,double total_time);
    void draw();                        //内部调用SDL函数

    //鼠标控制  返回true表示seek
    bool mouse_move(double mouse_x,double mouse_y);
    bool mouse_down(double mouse_x,double mouse_y);
    bool mouse_up(double mouse_x,double mouse_y);
    bool isDragging() const{return m_isDragging;}
    double getSeekTime() const{return m_seekTime;};              //指向的时间
    

private:
    //内部工具
    bool isMouseOnBar(double mouse_x,double mouse_y) const;
    double timeFromY(int mouse_y) const;        ////y方向上的鼠标位置换算时间


    //时间变量参数
    double m_totaltime = 0.0;
    double m_currenttime = 0.0;
    double m_seekTime = 0.0;            
    //条的参数
    int m_barTop =0;
    int m_barHeight = 0;

    //颜色参数
    SDL_Color m_bgColor{ 40, 40, 40, 200};
    SDL_Color m_progressColor{0,150,255,255};
    SDL_Color m_hoverColor{255,120,120,255};

    //状态参数
    bool m_isDragging = false;
    bool m_isHovering = false;

    //方向参数,true=0说明在底部，false=0在顶部
    bool m_fillFromBottom = true;
};