#pragma once

#include <QWidget>
#include <QBrush>
#include <QTime>
#include <QTimer>
#include <QLabel>
#include <QFile>
#include <QtWidgets\qboxlayout.h>

class RenderArea : public QWidget
{
	Q_OBJECT

public:
	RenderArea(QWidget* parent = 0);

public slots:
	void setFrame(QImage frame);

protected:
	void paintEvent(QPaintEvent* event) override;
	void keyPressEvent(QKeyEvent* ev);

signals:
	void signalKeyPress(QKeyEvent* ev);

private:

	QImage m_fImage;
	QVBoxLayout* mainLayout;

};