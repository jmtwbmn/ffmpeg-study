#include<ProgressBar.h>
#include<algorithm>

ProgressBar::ProgressBar(){}
ProgressBar::~ProgressBar(){}

void ProgressBar::init(int window_width,int window_height)
{
    m_windowWidth = window_width;
    m_windowHeight = window_height;
}

void ProgressBar::update(double current_time,double total_time)
{
    m_currentTime = current_time;
    m_totalTime = total_time;
}

void ProgressBar::draw(SDL_Renderer* renderer)
{
    if(!renderer || m_windowWidth == 0 || m_windowHeight ==0 )
    return;

    //计算进度条位置
    int bar_width = m_windowWidth -2 * m_marigin;
    int bar_y = m_windowHeight - m_barHeight - m_marigin;
    if(!renderer || m_windowWidth<=0 || m_windowHeight<=0) return;
    //绘制背景
    SDL_Rect bg_rect = {m_marigin,bar_y,bar_width,m_barHeight};
    SDL_SetRenderDrawColor(renderer,m_bgColor.r,m_bgColor.g,m_bgColor.b,m_bgColor.a);
    SDL_RenderFillRect(renderer,&bg_rect);

    //绘制进度
    if(m_totalTime >0 )
    {
        double progress = m_currentTime / m_totalTime;
        progress = std::min(1.0,std::max(0.0,progress));
        int progress_width = (int)(bar_width * progress);
        SDL_Rect progress_rect = {m_marigin,bar_y,progress_width,m_barHeight};

        //根据状态选择颜色
        SDL_Color& color = m_isHovering ? m_hoverColor : m_progressColor;
        SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
        SDL_RenderFillRect(renderer,&progress_rect);
    }
}

bool ProgressBar::handleMouseMove(int mouse_x,int mouse_y)
{
    m_isHovering = isMouseOnBar(mouse_x,mouse_y);
    //如果在拖动，更新seek时间
    if(m_isDragging)
    {
        m_seekTime = calculateTimeFromX(mouse_x);
        return true;
    }
    return false;
}

bool ProgressBar::handleMouseDown(int mouse_x,int mouse_y)
{
    if(isMouseOnBar(mouse_x,mouse_y))
    {
        m_isDragging = true;
        m_seekTime = calculateTimeFromX(mouse_x);
        return true;
    }
    return false;
}

bool ProgressBar::handleMouseUp(int mouse_x,int mouse_y)
{
    if(m_isDragging)
    {
        m_isDragging = false;
        return true;
    }
    return false;
}

bool ProgressBar::isMouseOnBar(int mouse_x,int mouse_y) const
{
    int bar_y = m_windowHeight - m_barHeight - m_marigin;
    return (mouse_y >= bar_y && mouse_y <= bar_y + m_barHeight&&
            mouse_x >= m_marigin && mouse_x <= m_windowWidth- m_marigin);
}
double ProgressBar::calculateTimeFromX(int mouse_x) const
{
    if(m_totalTime <= 0) return 0.0;
    int bar_width = m_windowWidth - 2*m_marigin;
    double progress = (mouse_x - m_marigin) / (double)bar_width;

    //限制范围在0~1
    progress = std::min(1.0,std::max(0.0,progress));
    return progress * m_totalTime;
}