

#include "CImageViewer.h"
#include <QtGui>

CImageViewer::CImageViewer(QList<CWpt::image_t>& images, QWidget *parent)
: QDialog(parent)
, images(images)
, idx(0)
, rectImage(0,0,100,100)
, rectClose(0,0,32,32)
, rectPrev(0,0,32,32)
, rectNext(0,0,32,32)
{
    setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);

    Qt::WindowFlags flags = windowFlags() & Qt::WindowType_Mask;
    setWindowFlags(flags | Qt::CustomizeWindowHint);
    showMaximized();

    if(images.isEmpty())
    {
        return;
    }


    setImageAtIdx(idx);
}

CImageViewer::~CImageViewer()
{

}

void CImageViewer::resizeEvent(QResizeEvent * e)
{
    QDialog::resizeEvent(e);
    setImageAtIdx(idx);
}

void CImageViewer::setImageAtIdx(int i)
{
    const QRect& rectScreen = rect();
    const QPoint& center    = rectScreen.center();
    QPixmap& pixmap         = images[i].pixmap;

    double width  = rectScreen.width() - 64;
    double height = rectScreen.height() - 64;

    if(pixmap.width() > width || pixmap.height() > height)
    {
        rectImage = pixmap.rect();

        if(pixmap.width() > width)
        {
            double ratio = width / pixmap.width();
            rectImage.setWidth(width);
            rectImage.setHeight(rectImage.height() * ratio);
        }

        if(pixmap.height() > height)
        {
            double ratio = height / pixmap.height();
            rectImage.setHeight(height);
            rectImage.setWidth(rectImage.width() * ratio);
        }

        rectImage.moveCenter(center);

    }
    else
    {
        rectImage = pixmap.rect();
        rectImage.moveCenter(center);
    }

    rectClose.moveCenter(rectImage.topRight());
    rectPrev.moveBottomLeft(rectImage.bottomLeft());
    rectNext.moveBottomRight(rectImage.bottomRight());
}

void CImageViewer::paintEvent(QPaintEvent * e)
{
    QDialog::paintEvent(e);

    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0,0,0,190));
    p.drawRect(rect());

    p.setPen(QPen(Qt::white, 11, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    p.setBrush(Qt::white);
    p.drawRect(rectImage);


    p.drawPixmap(rectImage, images[idx].pixmap);
    p.drawPixmap(rectClose, QPixmap(":/icons/iconClose32x32.png"));

    if(idx != (images.size() - 1))
    {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,128));
        p.drawRect(rectNext);
        p.drawPixmap(rectNext, QPixmap(":/icons/iconRight32x32.png"));
    }
    if(idx != 0)
    {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,128));
        p.drawRect(rectPrev);
        p.drawPixmap(rectPrev, QPixmap(":/icons/iconLeft32x32.png"));
    }

}

void CImageViewer::mousePressEvent(QMouseEvent * e)
{
    QPoint pos  = e->pos();
    if(rectClose.contains(pos))
    {
        reject();
    }
    else if(rectNext.contains(pos))
    {
        if(idx != (images.size() - 1))
        {
            setImageAtIdx(++idx);
            update();
        }
    }
    else if(rectPrev.contains(pos))
    {
        if(idx != 0)
        {
            setImageAtIdx(--idx);
            update();
        }
    }
}

