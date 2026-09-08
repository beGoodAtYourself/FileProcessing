#include <QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QImageReader>
#include <QElapsedTimer>
#include <QTimer>
#include "ImageConverterEngine.h"
#include "ImageConverterWorker.h"
#include "ImageConverterPanel.h"
#include "HeicDecoder.h"
#include "HeicEncoder.h"
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <psapi.h>
#endif

namespace
{
QStringList capturedLogs;
bool cancelRequested = false;

void captureLog(const char* message)
{
    capturedLogs.append(QString::fromUtf8(message));
}

void cancelAfterFirstFile(int current, int, int percent)
{
    if (current == 1 && percent == 100) cancelRequested = true;
}

QByteArray readBytes(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray();
    return file.readAll();
}

bool writeBytes(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

QImage sampleImage(bool alpha = true, int width = 64, int height = 32)
{
    QImage image(width, height, alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            image.setPixel(x, y, qRgba(x * 255 / width, y * 255 / height, 90,
                alpha ? (x % 3) * 127 : 255));
    return image;
}

int exportSvg(const QStringList& sources, const QString& directory = QString(),
              bool overwrite = false, int quality = 85)
{
    ImageConverterEngine engine;
    engine.addSourceFiles(sources);
    engine.setOutputDir(directory);
    engine.setTargetFormat(FORMAT_SVG);
    engine.setOverwrite(overwrite);
    engine.setQuality(quality);
    engine.setLogCallback(captureLog);
    return engine.convert();
}

void verifySvg(const QString& path, const QImage& expected)
{
    const QByteArray bytes = readBytes(path);
    QVERIFY(!bytes.isEmpty());
    QXmlStreamReader xml(bytes);
    QVERIFY(xml.readNextStartElement());
    QCOMPARE(xml.name().toString(), QString("svg"));
    QCOMPARE(xml.namespaceUri().toString(), QString("http://www.w3.org/2000/svg"));
    QCOMPARE(xml.attributes().value("version").toString(), QString("1.1"));
    QCOMPARE(xml.attributes().value("width").toInt(), expected.width());
    QCOMPARE(xml.attributes().value("height").toInt(), expected.height());
    QCOMPARE(xml.attributes().value("viewBox").toString(),
        QString("0 0 %1 %2").arg(expected.width()).arg(expected.height()));
    QVERIFY(xml.readNextStartElement());
    QCOMPARE(xml.name().toString(), QString("image"));
    QCOMPARE(xml.namespaceUri().toString(), QString("http://www.w3.org/2000/svg"));
    QCOMPARE(xml.attributes().value("x").toString(), QString("0"));
    QCOMPARE(xml.attributes().value("y").toString(), QString("0"));
    QCOMPARE(xml.attributes().value("width").toInt(), expected.width());
    QCOMPARE(xml.attributes().value("height").toInt(), expected.height());
    const QString uri = xml.attributes().value("http://www.w3.org/1999/xlink", "href").toString();
    const QString prefix = "data:image/png;base64,";
    QVERIFY(uri.startsWith(prefix));
    const QByteArray png = QByteArray::fromBase64(uri.mid(prefix.size()).toLatin1());
    QVERIFY(png.startsWith(QByteArray::fromHex("89504e470d0a1a0a")));
    const QImage embedded = QImage::fromData(png, "PNG").convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(embedded.size(), expected.size());
    QVERIFY(embedded == expected.convertToFormat(QImage::Format_ARGB32));
    xml.skipCurrentElement();
    QVERIFY(!xml.readNextStartElement());
    while (!xml.atEnd()) xml.readNext();
    QVERIFY2(!xml.hasError(), qPrintable(xml.errorString()));
}

QPushButton* buttonWithText(ImageConverterPanel& panel, const QString& text)
{
    foreach (QPushButton* button, panel.findChildren<QPushButton*>())
        if (button->text() == text) return button;
    return NULL;
}
}

class SvgExportTests : public QObject
{
    Q_OBJECT
private slots:
    void init() { capturedLogs.clear(); cancelRequested = false; }

    void formatMapping()
    {
        QCOMPARE((int)FORMAT_HEIC, 4);
        QCOMPARE((int)FORMAT_SVG, 5);
        QCOMPARE(ImageConverterEngine::getFormatExtension(FORMAT_SVG), QString("svg"));
        QCOMPARE(ImageConverterEngine::getFormatName(FORMAT_SVG), QString("SVG"));
    }

    void sourceFormats_data()
    {
        QTest::addColumn<QString>("extension");
        QTest::addColumn<bool>("alpha");
        const char* extensions[] = { "bmp", "jpg", "jpeg", "png", "ico", "heic", "heif" };
        for (int i = 0; i < 7; ++i)
            QTest::newRow(extensions[i]) << QString(extensions[i]) << false;
        QTest::newRow("transparent-png") << QString("png") << true;
        QTest::newRow("transparent-heic") << QString("heic") << true;
    }

    void sourceFormats()
    {
        QFETCH(QString, extension);
        QFETCH(bool, alpha);
        QTemporaryDir directory(QDir::currentPath() + "/format-XXXXXX");
        QVERIFY(directory.isValid());
        const QString path = directory.path() + "/photo.v2." + extension;
        const QImage image = sampleImage(alpha);
        const bool heic = extension == "heic" || extension == "heif";
        QString error;
        if (heic)
            QVERIFY2(HeicEncoder::encodeToFile(path, image, 85, &error), qPrintable(error));
        else
            QVERIFY(image.save(path));
        const QImage decoded = heic ? HeicDecoder::decodeFromFile(path, &error) : QImageReader(path).read();
        QVERIFY2(!decoded.isNull(), qPrintable(error));
        if (alpha) QVERIFY(decoded.hasAlphaChannel());
        const QByteArray original = readBytes(path);
        QCOMPARE(exportSvg(QStringList() << path), 1);
        verifySvg(directory.path() + "/photo.v2.svg", decoded);
        QCOMPARE(readBytes(path), original);
    }

    void qualityAndUnicodeDirectory()
    {
        QTemporaryDir directory(QDir::currentPath() + "/quality-XXXXXX");
        QVERIFY(directory.isValid());
        const QString input = directory.path() + "/source.png";
        const QImage image = sampleImage();
        QVERIFY(image.save(input));
        // UTF-8 bytes avoid dependence on the VS2013 source code page.
        const QString output = directory.path() + QString::fromUtf8("/\xe5\x9b\xbe\xe7\x89\x87 output/nested");
        QCOMPARE(exportSvg(QStringList() << input, output, false, 1), 1);
        const QByteArray lowQuality = readBytes(output + "/source.svg");
        QCOMPARE(exportSvg(QStringList() << input, output, true, 100), 1);
        QCOMPARE(readBytes(output + "/source.svg"), lowQuality);
        verifySvg(output + "/source.svg", image);
    }

    void collisionsAndFailuresContinue()
    {
        QTemporaryDir directory(QDir::currentPath() + "/batch-XXXXXX");
        QVERIFY(directory.isValid());
        const QString png = directory.path() + "/photo.png";
        const QString jpg = directory.path() + "/photo.jpg";
        const QString bad = directory.path() + "/bad.png";
        QVERIFY(sampleImage().save(png));
        QVERIFY(sampleImage(false).save(jpg));
        QVERIFY(writeBytes(bad, "not an image"));
        ImageConverterEngine engine;
        engine.setTargetFormat(FORMAT_SVG);
        engine.setLogCallback(captureLog);
        engine.addSourceFiles(QStringList() << bad << png << jpg);
        QCOMPARE(engine.convert(), 1);
        QCOMPARE(engine.getResults().size(), 3);
        QVERIFY(!engine.getResults()[0].success);
        QVERIFY(!QFileInfo::exists(directory.path() + "/bad.svg"));
        QCOMPARE(engine.getResults()[2].errorMessage, QString("Skipped (file exists)"));
        verifySvg(directory.path() + "/photo.svg", QImage(png));
        QVERIFY(capturedLogs.join("\n").contains("Failed to load image"));
        QVERIFY(capturedLogs.join("\n").contains("Skipped (file exists)"));
        const QByteArray existing = readBytes(directory.path() + "/photo.svg");
        QCOMPARE(engine.convert(), 0);
        QCOMPARE(readBytes(directory.path() + "/photo.svg"), existing);
        engine.setOverwrite(true);
        QCOMPARE(engine.convert(), 2);
        verifySvg(directory.path() + "/photo.svg", QImage(jpg));
    }

    void outputFailure()
    {
        QTemporaryDir directory(QDir::currentPath() + "/failure-XXXXXX");
        QVERIFY(directory.isValid());
        const QString input = directory.path() + "/image.png";
        QVERIFY(sampleImage().save(input));
        const QString blocker = directory.path() + "/blocker";
        QVERIFY(writeBytes(blocker, "original"));
        QCOMPARE(exportSvg(QStringList() << input, blocker + "/nested", true), 0);
        QVERIFY(capturedLogs.join("\n").contains("Failed to create output directory"));
        QCOMPARE(readBytes(blocker), QByteArray("original"));
        QVERIFY(!QFileInfo::exists(directory.path() + "/image.svg"));
        QVERIFY(QDir().mkdir(directory.path() + "/image.svg"));
        const QStringList before = QDir(directory.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
        QCOMPARE(exportSvg(QStringList() << input, directory.path(), true), 0);
        QVERIFY(capturedLogs.join("\n").contains("Failed to open SVG output"));
        QCOMPARE(QDir(directory.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot), before);
    }

    void failedCommitPreservesExistingFile()
    {
#ifdef Q_OS_WIN
        QTemporaryDir directory(QDir::currentPath() + "/locked-XXXXXX");
        QVERIFY(directory.isValid());
        const QString input = directory.path() + "/image.png";
        const QString output = directory.path() + "/image.svg";
        QVERIFY(sampleImage().save(input));
        QVERIFY(writeBytes(output, "existing target must survive"));
        const QStringList before = QDir(directory.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
        HANDLE locked = CreateFileW(reinterpret_cast<LPCWSTR>(output.utf16()), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        QVERIFY(locked != INVALID_HANDLE_VALUE);
        const int converted = exportSvg(QStringList() << input, directory.path(), true);
        CloseHandle(locked);
        QCOMPARE(converted, 0);
        QVERIFY2(capturedLogs.join("\n").contains("Failed to commit SVG output"), qPrintable(capturedLogs.join("\n")));
        QCOMPARE(readBytes(output), QByteArray("existing target must survive"));
        QCOMPARE(QDir(directory.path()).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot), before);
#else
        QSKIP("Windows file-sharing semantics are needed for this commit-failure test.");
#endif
    }

    void cancellationAtFileBoundary()
    {
        QTemporaryDir directory(QDir::currentPath() + "/cancel-XXXXXX");
        QVERIFY(directory.isValid());
        const QString first = directory.path() + "/first.png";
        const QString second = directory.path() + "/second.png";
        QVERIFY(sampleImage().save(first));
        QVERIFY(sampleImage().save(second));
        ImageConverterEngine engine;
        engine.setTargetFormat(FORMAT_SVG);
        engine.addSourceFiles(QStringList() << first << second);
        engine.setCancelFlag(&cancelRequested);
        engine.setProgressCallback(cancelAfterFirstFile);
        QCOMPARE(engine.convert(), 1);
        QCOMPARE(engine.getResults().size(), 1);
        verifySvg(directory.path() + "/first.svg", QImage(first));
        QVERIFY(!QFileInfo::exists(directory.path() + "/second.svg"));
    }

    void existingTargets_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<int>("quality");
        for (int format = FORMAT_BMP; format <= FORMAT_HEIC; ++format)
            QTest::newRow(qPrintable(ImageConverterEngine::getFormatName((ImageFormat)format))) << format << 85;
        QTest::newRow("JPEG-low-quality") << (int)FORMAT_JPEG << 10;
        QTest::newRow("HEIC-low-quality") << (int)FORMAT_HEIC << 10;
    }

    void existingTargets()
    {
        QFETCH(int, format);
        QFETCH(int, quality);
        QTemporaryDir directory(QDir::currentPath() + "/regression-XXXXXX");
        QVERIFY(directory.isValid());
        const QString input = directory.path() + "/image.png";
        QVERIFY(sampleImage(false).save(input));
        ImageConverterEngine engine;
        engine.addSourceFile(input);
        engine.setOutputDir(directory.path() + "/out");
        engine.setTargetFormat((ImageFormat)format);
        engine.setQuality(quality);
        QCOMPARE(engine.convert(), 1);
        const QString output = engine.getResults()[0].outputPath;
        QCOMPARE(QFileInfo(output).suffix(), ImageConverterEngine::getFormatExtension((ImageFormat)format));
        const QImage decoded = format == FORMAT_HEIC ? HeicDecoder::decodeFromFile(output) : QImageReader(output).read();
        QCOMPARE(decoded.size(), QSize(64, 32));
    }

    void panelSingleAndBatch()
    {
        QTemporaryDir directory(QDir::currentPath() + "/panel-XXXXXX");
        QVERIFY(directory.isValid());
        const QString first = directory.path() + "/single.png";
        const QString second = directory.path() + "/batch.bmp";
        QVERIFY(sampleImage().save(first));
        QVERIFY(sampleImage(false).save(second));
        ImageConverterPanel panel;
        panel.show();
        QComboBox* formats = panel.findChild<QComboBox*>();
        QSpinBox* quality = panel.findChild<QSpinBox*>();
        QVERIFY(formats && quality);
        QCOMPARE(formats->currentData().toInt(), (int)FORMAT_JPEG);
        QCOMPARE(formats->count(), 6);
        formats->setCurrentIndex(formats->findData((int)FORMAT_SVG));
        QVERIFY(quality->isHidden());
        QLabel* hint = NULL;
        foreach (QLabel* label, panel.findChildren<QLabel*>())
            if (label->text().contains("original resolution")) hint = label;
        QVERIFY(hint && !hint->isHidden());
        QVERIFY(hint->wordWrap());
        QVERIFY(QDir().mkpath("artifacts"));
        QCoreApplication::processEvents();
        QVERIFY(panel.grab().save("artifacts/svg-panel.png"));
        formats->setCurrentIndex(formats->findData((int)FORMAT_HEIC));
        QVERIFY(!quality->isHidden());
        QVERIFY(hint->isHidden());
        formats->setCurrentIndex(formats->findData((int)FORMAT_JPEG));
        QVERIFY(!quality->isHidden());
        formats->setCurrentIndex(formats->findData((int)FORMAT_SVG));
        QLineEdit* source = NULL;
        foreach (QLineEdit* edit, panel.findChildren<QLineEdit*>())
            if (edit->placeholderText().startsWith("Select an image")) source = edit;
        QVERIFY(source);
        source->setText(first);
        QPushButton* convert = buttonWithText(panel, "Convert");
        QPushButton* cancel = buttonWithText(panel, "Cancel");
        QVERIFY(convert && cancel);
        QVERIFY(QMetaObject::invokeMethod(&panel, "onStartConversion", Qt::DirectConnection));
        QTRY_VERIFY(convert->isEnabled());
        QVERIFY(!cancel->isEnabled());
        verifySvg(directory.path() + "/single.svg", QImage(first));
        panel.findChild<QTabWidget*>()->setCurrentIndex(1);
        panel.findChild<QListWidget*>()->addItem(second);
        QVERIFY(QMetaObject::invokeMethod(&panel, "onStartConversion", Qt::DirectConnection));
        QTRY_VERIFY(convert->isEnabled());
        verifySvg(directory.path() + "/batch.svg", QImage(second));
        QVERIFY(panel.findChild<QTextEdit*>()->toPlainText().contains("1/1 files converted successfully"));
    }

    void workerLargeImageAndStandaloneArtifact()
    {
        QVERIFY(QDir().mkpath("artifacts/source"));
        QVERIFY(QDir().mkpath("artifacts/standalone"));
        const QString input = QDir::currentPath() + "/artifacts/source/large.png";
        const QImage image = sampleImage(true, 4096, 2048);
        QVERIFY(image.save(input));
        ImageConverterWorker worker;
        worker.setSourceFiles(QStringList() << input);
        worker.setTargetFormat(FORMAT_SVG);
        worker.setOutputDir(QDir::currentPath() + "/artifacts/source");
        worker.setOverwrite(true);
        QSignalSpy finished(&worker, SIGNAL(conversionFinished(int,int)));
        QSignalSpy progress(&worker, SIGNAL(progressChanged(int,int,int)));
        QTimer timer;
        timer.setInterval(10);
        QSignalSpy ticks(&timer, SIGNAL(timeout()));
        QElapsedTimer elapsed;
        elapsed.start();
        timer.start();
        worker.start();
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(worker.wait(10000));
        timer.stop();
        QCOMPARE(finished[0][0].toInt(), 1);
        QVERIFY(progress.count() >= 2);
        QVERIFY(ticks.count() > 0);
        qDebug() << "4096x2048 SVG export ms:" << elapsed.elapsed() << "UI timer ticks:" << ticks.count();
#ifdef Q_OS_WIN
        PROCESS_MEMORY_COUNTERS memory;
        memset(&memory, 0, sizeof(memory));
        memory.cb = sizeof(memory);
        QVERIFY(GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)));
        qDebug() << "Process peak working set bytes:" << (qulonglong)memory.PeakWorkingSetSize;
#endif
        verifySvg("artifacts/source/large.svg", image);
        // Keep a small, transparent SVG by itself for actual browser verification.
        const QImage preview = sampleImage(true, 512, 256);
        QVERIFY(preview.save("artifacts/source/preview.png"));
        QCOMPARE(exportSvg(QStringList() << "artifacts/source/preview.png", "artifacts/source", true), 1);
        QVERIFY(writeBytes("artifacts/standalone/preview.svg", readBytes("artifacts/source/preview.svg")));
        verifySvg("artifacts/standalone/preview.svg", preview);
    }
};

QTEST_MAIN(SvgExportTests)
#include "svg_export_tests.moc"
