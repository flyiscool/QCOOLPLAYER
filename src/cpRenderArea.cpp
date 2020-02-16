#include "stdafx.h"
#include "cpRenderArea.h"
#include <QPainter>


RenderArea::RenderArea(QWidget* parent)
	: QWidget(parent)
{

	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);

}

void RenderArea::paintEvent(QPaintEvent* /* event */)
{
	QPainter painter(this);
	painter.setBrush(Qt::black);
	painter.drawRect(0, 0, this->width(), this->height());

	if (m_fImage.size().width() <= 0) return;



	QImage img = m_fImage.scaled(this->size(), Qt::KeepAspectRatio);
	//QImage img = m_fImage;
	//QImage img = m_fImage.scaled(screenRect.size(), Qt::KeepAspectRatio);

	int x = this->width() - img.width();
	int y = this->height() - img.height();

	x /= 2;
	y /= 2;

	painter.drawImage(QPoint(x, y), img);
}

void RenderArea::setFrame(QImage frame)
{
	m_fImage = frame;
	update();
}
