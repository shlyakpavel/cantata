/*
 * Cantata
 *
 * Copyright (c) 2011-2015 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "multipagewidget.h"
#include "support/icon.h"
#include "support/utils.h"
#include "support/squeezedtextlabel.h"
#include "listview.h"
#include "sizewidget.h"
#include "singlepagewidget.h"
#include <QGridLayout>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>

class SelectorButton : public QToolButton
{
public:
    SelectorButton(const QString &t, const QString &s, const Icon &icn, QWidget *p)
        : QToolButton(p)
    {
        QGridLayout *layout=new QGridLayout(this);
        icon=new QLabel(this);
        mainText=new SqueezedTextLabel(this);
        subText=new SqueezedTextLabel(this);
        QFont f=mainText->font();
        subText->setFont(Utils::smallFont(f));
        f.setBold(true);
        mainText->setFont(f);
        int size=mainText->sizeHint().height()+subText->sizeHint().height()+Utils::layoutSpacing(this);
        size+=12;
        icon->setFixedSize(size, size);
        layout->addWidget(icon, 0, 0, 2, 1);
        layout->addWidget(mainText, 0, 1, 1, 1);
        layout->addWidget(subText, 1, 1, 1, 1);
        mainText->setAlignment(Qt::AlignBottom);
        subText->setAlignment(Qt::AlignTop);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(icn.pixmap(64, 64).scaled(icon->width()-2, icon->height()-2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        setAutoRaise(true);
        setLayout(layout);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        mainText->setText(t);
        subText->setText(s);
        setMinimumHeight(size+18);
    }

    void setSubText(const QString &str)
    {
        subText->setText(str);
    }

private:
    SqueezedTextLabel *mainText;
    SqueezedTextLabel *subText;
    QLabel *icon;
};

MultiPageWidget::MultiPageWidget(QWidget *p)
    : QStackedWidget(p)
{
    mainPage=new QWidget(this);
    QVBoxLayout *mainLayout=new QVBoxLayout(mainPage);
    view=new ListView(mainPage);
    infoLabel=new QLabel(mainPage);
    sizer=new SizeWidget(mainPage);
    QVBoxLayout *layout=new QVBoxLayout(view);

    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
    view->setLayout(layout);
    mainPage->setLayout(mainLayout);
    mainLayout->addWidget(view);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(infoLabel);
    infoLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    mainLayout->addWidget(sizer);
    addWidget(mainPage);
}

MultiPageWidget::~MultiPageWidget()
{
}

void MultiPageWidget::setView(int v)
{
    for (int i=0; i<count(); ++i) {
        if (dynamic_cast<SinglePageWidget *>(widget(i))) {
            static_cast<SinglePageWidget *>(widget(i))->setView(v);
        }
    }
}

void MultiPageWidget::focusSearch()
{
    QWidget *w=currentWidget();
    if (dynamic_cast<SinglePageWidget *>(w)) {
        static_cast<SinglePageWidget *>(w)->focusSearch();
    }
}

QStringList MultiPageWidget::selectedFiles(bool allowPlaylists) const
{
    if (onMainPage() || !dynamic_cast<SinglePageWidget *>(currentWidget())) {
        return QStringList();
    }
    return static_cast<SinglePageWidget *>(currentWidget())->selectedFiles(allowPlaylists);
}

QList<Song> MultiPageWidget::selectedSongs(bool allowPlaylists) const
{
    if (onMainPage() || !dynamic_cast<SinglePageWidget *>(currentWidget())) {
        return QList<Song>();
    }
    return static_cast<SinglePageWidget *>(currentWidget())->selectedSongs(allowPlaylists);
}

void MultiPageWidget::addSelectionToPlaylist(const QString &name, bool replace, quint8 priorty)
{
    if (onMainPage() || !dynamic_cast<SinglePageWidget *>(currentWidget())) {
        return;
    }
    return static_cast<SinglePageWidget *>(currentWidget())->addSelectionToPlaylist(name, replace, priorty);
}

void MultiPageWidget::setInfoText(const QString &text)
{
    infoLabel->setText(text);
}

void MultiPageWidget::addPage(const QString &name, const QString &icon, const QString &text, const QString &subText, QWidget *widget)
{
    Icon i;
    i.addFile(":"+icon);
    addPage(name, i, text, subText, widget);
}

void MultiPageWidget::addPage(const QString &name, const Icon &icon, const QString &text, const QString &subText, QWidget *widget)
{
    if (entries.contains(name)) {
        return;
    }
    Entry e(new SelectorButton(text, subText, icon, view), widget);
    static_cast<QVBoxLayout *>(view->layout())->insertWidget(view->layout()->count()-1, e.btn);
    addWidget(widget);
    entries.insert(name, e);
    connect(e.btn, SIGNAL(clicked()), SLOT(setPage()));
    infoLabel->setVisible(false);
    if (dynamic_cast<SinglePageWidget *>(widget)) {
        connect(static_cast<SinglePageWidget *>(widget), SIGNAL(close()), this, SLOT(showMainView()));
    }
}

void MultiPageWidget::removePage(const QString &name)
{
    QMap<QString, Entry>::Iterator it=entries.find(name);
    if (it==entries.end()) {
        return;
    }
    if (it.value().page==currentWidget()) {
        setCurrentWidget(mainPage);
    }
    static_cast<QVBoxLayout *>(view->layout())->removeWidget(it.value().btn);
    it.value().btn->deleteLater();
    entries.erase(it);
}

void MultiPageWidget::updatePageSubText(const QString &name, const QString &text)
{
    QMap<QString, Entry>::Iterator it=entries.find(name);
    if (it==entries.end()) {
        return;
    }
    it.value().btn->setSubText(text);
}

void MultiPageWidget::showMainView()
{
    setCurrentWidget(mainPage);
}

void MultiPageWidget::setPage()
{
    QToolButton *btn=qobject_cast<QToolButton *>(sender());

    if (!btn) {
        return;
    }

    QMap<QString, Entry>::ConstIterator it=entries.constBegin();
    QMap<QString, Entry>::ConstIterator end=entries.constEnd();

    for (; it!=end; ++it) {
        if (it.value().btn==btn) {
            setCurrentWidget(it.value().page);
            return;
        }
    }
}

void MultiPageWidget::sortItems()
{
    QList<QString> keys=entries.keys();
    qSort(keys);
    infoLabel->setVisible(0==entries.count());
    QVBoxLayout *layout=static_cast<QVBoxLayout *>(view->layout());

    QMap<QString, Entry>::ConstIterator it=entries.constBegin();
    QMap<QString, Entry>::ConstIterator end=entries.constEnd();

    for (; it!=end; ++it) {
        layout->removeWidget(it.value().btn);
    }

    foreach (const QString &key, keys) {
        layout->insertWidget(view->layout()->count()-1, entries[key].btn);
    }
}