// This file is part of Heimer.
// Copyright (C) 2018 Jussi Lind <jussi.lind@iki.fi>
//
// Heimer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Heimer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Heimer. If not, see <http://www.gnu.org/licenses/>.

#include "exporttopngdialog.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
const QString FILE_EXTENSION = ".png";
const int MIN_IMAGE_SIZE = 0;
const int MAX_IMAGE_SIZE = 99999;
}

ExportToPNGDialog::ExportToPNGDialog(QWidget * parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Export to PNG Image"));
    setMinimumWidth(480);
    initWidgets();

    connect(m_filenameButton, &QPushButton::clicked, [=] () {
        auto filename = QFileDialog::getSaveFileName(this,
            tr("Export As"),
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
            tr("PNG Files") + " (*" + FILE_EXTENSION + ")");

        if (filename.isEmpty())
        {
            return;
        }

        if (!filename.toLower().endsWith(FILE_EXTENSION))
        {
            filename += FILE_EXTENSION;
        }

        m_filenameLineEdit->setText(filename);
    });

    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::close);

    connect(m_exportButton, &QPushButton::clicked, [=] () {
        m_exportButton->setEnabled(false);
        m_progressBar->setValue(50);
        emit pngExportRequested(m_filenameLineEdit->text(), QSize(m_imageWidthSpinBox->value(), m_imageHeightSpinBox->value()));
    });

    // The ugly cast is needed because there are QSpinBox::valueChanged(int) and QSpinBox::valueChanged(QString)
    // In Qt > 5.10 one can use QOverload<int>::of(...)
    connect(m_imageWidthSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&] (int value) {
        if (m_enableSpinBoxConnection) {
            m_enableSpinBoxConnection = false;
            m_imageHeightSpinBox->setValue(value / m_aspectRatio);
            m_enableSpinBoxConnection = true;
        }
    });

    connect(m_imageHeightSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), [&] (int value) {
        if (m_enableSpinBoxConnection) {
            m_enableSpinBoxConnection = false;
            m_imageWidthSpinBox->setValue(value * m_aspectRatio);
            m_enableSpinBoxConnection = true;
        }
    });

    connect(m_filenameLineEdit, &QLineEdit::textChanged, this, &ExportToPNGDialog::validate);

    connect(m_imageWidthSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ExportToPNGDialog::validate);

    connect(m_imageHeightSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ExportToPNGDialog::validate);
}

void ExportToPNGDialog::setImageSize(QSize size)
{
    m_enableSpinBoxConnection = false;

    // Set a bit bigger size as default. With the original
    // size the result would be a bit blurry.
    size *= 2;

    m_imageWidthSpinBox->setValue(size.width());
    m_imageHeightSpinBox->setValue(size.height());

    m_aspectRatio = float(size.width()) / size.height();

    m_enableSpinBoxConnection = true;
}

int ExportToPNGDialog::exec()
{
    m_progressBar->setValue(0);

    validate();

    return QDialog::exec();
}

void ExportToPNGDialog::finishExport()
{
    m_progressBar->setValue(100);

    QTimer::singleShot(1000, this, &QDialog::accept);
}

void ExportToPNGDialog::validate()
{
    m_progressBar->setValue(0);

    m_exportButton->setEnabled(
         m_imageHeightSpinBox->value() > MIN_IMAGE_SIZE && // Intentionally open intervals
         m_imageHeightSpinBox->value() < MAX_IMAGE_SIZE &&
         m_imageWidthSpinBox->value() > MIN_IMAGE_SIZE &&
         m_imageWidthSpinBox->value() < MAX_IMAGE_SIZE &&
         !m_filenameLineEdit->text().isEmpty()
    );
}

void ExportToPNGDialog::initWidgets()
{
    auto mainLayout = new QVBoxLayout(this);
    const auto filenameLabel = new QLabel("<b>" + tr("Filename") + "</b>");
    mainLayout->addWidget(filenameLabel);

    auto filenameLayout = new QHBoxLayout;
    m_filenameLineEdit = new QLineEdit;
    filenameLayout->addWidget(m_filenameLineEdit);
    m_filenameButton = new QPushButton;
    m_filenameButton->setText(tr("Export as.."));
    filenameLayout->addWidget(m_filenameButton);
    mainLayout->addLayout(filenameLayout);

    const auto imageSizeLabel = new QLabel("<b>" + tr("Image Size") + "</b>");
    mainLayout->addWidget(imageSizeLabel);
    auto imageSizeLayout = new QHBoxLayout;
    const auto imageWidthLabel = new QLabel(tr("Width (px):"));
    imageSizeLayout->addWidget(imageWidthLabel);
    m_imageWidthSpinBox = new QSpinBox;
    m_imageWidthSpinBox->setMinimum(MIN_IMAGE_SIZE);
    m_imageWidthSpinBox->setMaximum(MAX_IMAGE_SIZE);
    imageSizeLayout->addWidget(m_imageWidthSpinBox);
    const auto imageHeightLabel = new QLabel(tr("Height (px):"));
    imageSizeLayout->addWidget(imageHeightLabel);
    m_imageHeightSpinBox = new QSpinBox;
    m_imageHeightSpinBox->setMinimum(MIN_IMAGE_SIZE);
    m_imageHeightSpinBox->setMaximum(MAX_IMAGE_SIZE);
    imageSizeLayout->addWidget(m_imageHeightSpinBox);
    mainLayout->addLayout(imageSizeLayout);

    auto progressBarLayout = new QHBoxLayout;
    m_progressBar = new QProgressBar;
    m_progressBar->setEnabled(false);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    progressBarLayout->addWidget(m_progressBar);
    mainLayout->addLayout(progressBarLayout);

    auto buttonLayout = new QHBoxLayout;
    m_cancelButton = new QPushButton;
    m_cancelButton->setText(tr("Cancel"));
    buttonLayout->addWidget(m_cancelButton);
    m_exportButton = new QPushButton;
    m_exportButton->setText(tr("Export"));
    m_exportButton->setEnabled(false);
    buttonLayout->addWidget(m_exportButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}
