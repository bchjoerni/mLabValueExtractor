#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>

#include <fstream>
#include <string>
#include <limits>
#include <thread>
#include <atomic>

namespace Ui
{
    class mainWindow;
}

static std::atomic<bool> _readWrite_;

class mainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit mainWindow( QWidget *parent = 0 );
    ~mainWindow();

private slots:
    void selectInputFile();
    void selectOutputFile();
    void extractValues();
    void clearInfoText();
    void exit();

private:
    bool allValuesSet();
    void addInfoText( const QString& text );    
    void writeTextToFile( const std::string& text,
                          const std::string& fileName );
    static void read1Value( std::ifstream* stream, std::string* text,
                            const std::string& value1,
                            const std::string& delimiter,
                            long long start, long long stop,
                            bool useKomma, long long* numValues );
    static void read2Values( std::ifstream* stream, std::string* text,
                             const std::string& value1,
                             const std::string& value2,
                             const std::string& delimiter,
                             long long start, long long stop,
                             bool useKomma, long long* numValues );
    static std::string replacePointWithKomma( std::string& text );
    static void writeToFile( std::ofstream* stream,
                             const std::string& fileName );
    std::string getDelimiter();

    Ui::mainWindow *_ui;

    const QString DELIMITER_TABULATOR  = "    (tabulator)";
    const QString DELIMITER_SEMIKOLON  = "; (semikolon)";
    const QString DELIMITER_KOMMA      = ", (komma)";
    const QString DELIMITER_WHITESPACE = "  (whitespace)";

    const QString USE_POINT = ". (use point)";
    const QString USE_KOMMA = ", (use komma)";
};

#endif // MAINWINDOW_H
