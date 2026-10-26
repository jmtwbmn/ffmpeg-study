#pragma once

#include<SDL2/SDL.h>

class ProgressBar{

public:
    ProgressBar();
    ~ProgressBar();

    //初始化进度条
    void init(int window_width,int window_height);
    
    //更新进度信息
    void update(double current_time,double total_time);

    //绘制进度条
    void draw(SDL_Renderer* renderer);

    //管理鼠标事件，返回true表示seek
    bool handleMouseMove(int mouse_x,int mouse_y);
    bool handleMouseDown(int mouse_x,int mouse_y);
    bool handleMouseUp(int mouse_x,int mouse_y);

    //获取点击/拖动的对应时间
    double getSeekTime() const{return m_seekTime;}

    //判断是否拖动
    bool isDragging() const {return m_isDragging;}

private:
    //窗口尺寸
    int m_windowWidth = 0;
    int m_windowHeight = 0;

    //进度条配置
    int m_barHeight = 20;
    int m_marigin = 10;

    //颜色
    SDL_Color m_bgColor = {50,50,50,255};           //背景色
    SDL_Color m_progressColor = {0,150,255,255};            //进度色
    SDL_Color m_hoverColor = {255,100,100,255};             //悬停色

    //状态
    double m_currentTime = 0.0;
    double m_totalTime = 0.0;
    bool m_isHovering = false;
    bool m_isDragging = false;
    double m_seekTime = 0.0;

    //辅助函数
    bool isMouseOnBar(int mouse_x,int mouse_y) const;
    double calculateTimeFromX(int mouse_x) const;
};
