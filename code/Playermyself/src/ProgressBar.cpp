#include "ProgressBar.h"
#include<iostream>
#include<SDL2/SDL.h>

ProgressBar::ProgressBar(){};
ProgressBar::~ProgressBar(){};


//绘制条
void ProgressBar::progress_init(double windowwidth,double windowheight,SDL_Renderer* renderer)
{   
    m_windowWidth = windowwidth;
    m_windnowHeight = windowheight;
    m_renderer = renderer;
    
    const int bar_width = 20;
    const int marigin = 10;

    m_bgrect.x = (int)windowwidth - bar_width - marigin;
    m_bgrect.y = marigin;
    m_bgrect.w = bar_width;
    m_bgrect.h = (int)windowheight -marigin*2;
   
}
void ProgressBar::updateprogress(double current_time,double total_time)
{
    m_currenttime = current_time;
    m_totaltime = total_time;
   
}  
void ProgressBar::draw()
{
    //目前播放到的部分
    if(m_totaltime<=0.0) return;        //防止分母为0
    int fill_h = (int)(m_bgrect.h * (m_currenttime / m_totaltime));
    SDL_Rect fill = m_bgrect;
    fill.h = fill_h;
    fill.y = m_bgrect.y+m_bgrect.h - fill.h; //涨水似下往上
    //调用SDL函数
    //开启混合模式
    SDL_SetRenderDrawBlendMode(m_renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer,m_bgColor.r,m_bgColor.g,m_bgColor.b,m_bgColor.a);          //设置颜色
    SDL_RenderFillRect(m_renderer,&m_bgrect);       //设置背景
    //这里加入判断两种颜色而二选一，正常播放和悬停
    SDL_Color& c = (m_isHovering || m_isDragging)? m_hoverColor : m_progressColor;
    SDL_SetRenderDrawColor(m_renderer,c.r,c.g,c.b,c.a);
    SDL_RenderFillRect(m_renderer,&fill);           //已播放部分的背景
}

//鼠标控制函数
bool ProgressBar::mouse_down(double mouse_x,double mouse_y)
{  
    //记录鼠标x,y是否在进度条内
    if(isMouseOnBar(mouse_x,mouse_y))
    {
        m_isHovering = true;
        m_isDragging = true;
        m_seekTime = timeFromY(mouse_y);
        return true;
    }
    return false;
}
bool ProgressBar::mouse_move(double mouse_x,double mouse_y)
{   
    //记录鼠标x,y是否在进度条内
    m_isHovering = isMouseOnBar(mouse_x,mouse_y);
   if(m_isDragging)
   {    //计算此时的进度
    m_seekTime = timeFromY(mouse_y);
    return true;
   }
    return false;
}
bool ProgressBar::mouse_up(double mouse_x,double mouse_y)
{   
//记录鼠标x,y是否在进度条内
   m_isHovering = isMouseOnBar(mouse_x,mouse_y);
   if(!m_isHovering) return false;
   if(!m_isDragging) return false;
   m_isDragging = false;
    m_seekTime = timeFromY(mouse_y);
   return true;
}

bool ProgressBar::isMouseOnBar(double mouse_x,double mouse_y) const
{
   //记录鼠标x,y是否在进度条内
    if(mouse_x >= m_bgrect.x&& mouse_x <= m_bgrect.x+m_bgrect.w&&
       mouse_y >= m_bgrect.y && mouse_y <= m_bgrect.y+m_bgrect.h)
    return true;
    else
    return false;
}

//将鼠标在进度条的位置转换成时间
double ProgressBar::timeFromY(int mouse_y) const
{
    if(m_bgrect.h <= 0 || m_totaltime<= 0.0) return 0.0;

    double progress = (m_bgrect.h+m_bgrect.y - mouse_y) / m_bgrect.h;

    if(progress < 0.0) progress = 0.0;
    if(progress > 1.0) progress = 1.0;
    return progress * m_totaltime;
}