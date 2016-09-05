#include "mainwindow.h"
#include "ui_mainwindow.h"

mainWindow::mainWindow( QWidget *parent ) :
    QMainWindow( parent ),
    _ui( new Ui::mainWindow )
{
    _ui->setupUi(this);

    connect( _ui->btn_selectInputFile, SIGNAL( clicked() ), this,
             SLOT( selectInputFile() ) );
    connect( _ui->btn_selectOutputFile, SIGNAL( clicked() ), this,
             SLOT( selectOutputFile() ) );
    connect( _ui->btn_extract, SIGNAL( clicked() ), this,
             SLOT( extractValues() ) );
    connect( _ui->btn_clearInfo, SIGNAL( clicked() ), this,
             SLOT( clearInfoText() ) );
    connect( _ui->btn_exit, SIGNAL( clicked() ), this,
             SLOT( exit() ) );

    _ui->cob_delimiter->addItem( DELIMITER_TABULATOR );
    _ui->cob_delimiter->addItem( DELIMITER_SEMIKOLON );
    _ui->cob_delimiter->addItem( DELIMITER_KOMMA );
    _ui->cob_delimiter->addItem( DELIMITER_WHITESPACE );

    _ui->cob_pointOrKomma->addItem( USE_KOMMA );
    _ui->cob_pointOrKomma->addItem( USE_POINT );

    _readWrite_.store( false );
    addInfoText( "Program start." );
}

mainWindow::~mainWindow()
{
    delete _ui;
}

void mainWindow::addInfoText( const QString& text )
{
    _ui->txt_info->append( QTime::currentTime().toString( "hh:mm:ss,zzz:" ) +
                           text );
}

void mainWindow::selectInputFile()
{
    _ui->txt_inputFile->setText(
                QFileDialog::getOpenFileName( this, "Select data file", "",
                                              "text files (*.txt)" ) );
}

void mainWindow::selectOutputFile()
{
    QString fileName = _ui->txt_inputFile->text().replace( ".txt", "" );
    if( !fileName.isEmpty() )
    {
        if( _ui->rad_oneValue->isChecked() )
        {
            fileName += "_" + _ui->txt_firstValue->text();
        }
        if( _ui->rad_twoValues->isChecked() )
        {
            fileName += "_" + _ui->txt_secondValue->text();
        }
        fileName += ".txt";
    }
    _ui->txt_outputFile->setText(
                QFileDialog::getSaveFileName( this, "Save file", fileName,
                                              "text files (*.txt)" ) );
}

bool mainWindow::allValuesSet()
{
    if( _ui->txt_inputFile->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error",
                              "No input data file selected!" );
        return false;
    }
    if( _ui->txt_firstValue->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error",
                              "No value specified!" );
        return false;
    }
    if( _ui->rad_twoValues->isChecked()
            && _ui->txt_secondValue->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error",
                              "Second value is not specified!" );
        return false;
    }
    if( _ui->txt_outputFile->text().isEmpty() )
    {
        QMessageBox::warning( this, "Error",
                              "No output file selected!" );
        return false;
    }
    return true;
}

std::string mainWindow::getDelimiter()
{
    if( _ui->cob_delimiter->currentText() == DELIMITER_TABULATOR )
    {
        return "\t";
    }
    else if( _ui->cob_delimiter->currentText() == DELIMITER_SEMIKOLON )
    {
        return ";";
    }
    else if( _ui->cob_delimiter->currentText() == DELIMITER_KOMMA )
    {
        return ",";
    }
    else if( _ui->cob_delimiter->currentText() == DELIMITER_WHITESPACE )
    {
        return " ";
    }
    else
    {
        return ";";
    }
}

void mainWindow::extractValues()
{
    _ui->btn_extract->setEnabled( false );
    if( !allValuesSet() )
    {
        addInfoText( "Aborted, incorrect user input!" );
        _ui->btn_extract->setEnabled( true );
        return;
    }

    long long numValues = 0;
    long long startValue = _ui->rad_inRange->isChecked() ?
                _ui->spb_startValue->value() : 1;
    long long stopValue = _ui->rad_inRange->isChecked() ?
                _ui->spb_stopValue->value() :
                std::numeric_limits<long long>::max();
    if( stopValue == 0 )
    {
        stopValue = std::numeric_limits<long long>::max();
    }
    std::string value1 = _ui->txt_firstValue->text().toStdString();
    std::string value2 = _ui->txt_secondValue->text().toStdString();

    std::ifstream inStream;
    inStream.open( _ui->txt_inputFile->text().toStdString().c_str(),
                   std::ios_base::in );
    if( !inStream.is_open() )
    {
        addInfoText( "Error: could not open input data file!" );
        _ui->btn_extract->setEnabled( true );
        return;
    }
    addInfoText( "Reading input data file..." );
    std::string text;
    _readWrite_.store( true );
    std::thread readThread;
    bool useKomma = (_ui->cob_pointOrKomma->currentText() == USE_KOMMA);
    std::string delimiter = getDelimiter();

    if( _ui->rad_oneValue->isChecked() )
    {
        readThread = std::thread( read1Value, &inStream, &text, value1,
                                  delimiter, startValue, stopValue, useKomma,
                                  &numValues );
    }
    else if( _ui->rad_twoValues->isChecked() )
    {
        readThread = std::thread( read2Values, &inStream, &text, value1, value2,
                                  delimiter, startValue, stopValue, useKomma,
                                  &numValues );
    }

    while( _readWrite_.load() )
    {
        qApp->processEvents();
    }
    readThread.join();
    inStream.close();

    if( text == "" )
    {
        addInfoText( "No values read!" );
        _ui->btn_extract->setEnabled( true );
        return;
    }
    else
    {
        addInfoText( "Data file read, " + QString::number( numValues )
                     + " values extracted." );
    }

    writeTextToFile( text, _ui->txt_outputFile->text().toStdString() );
    addInfoText( "Extraction finished :)" );
    _ui->btn_extract->setEnabled( true );
    _ui->txt_firstValue->setText( "" );
    _ui->txt_secondValue->setText( "" );
    _ui->txt_outputFile->setText( "" );
}

void mainWindow::read1Value( std::ifstream* stream, std::string* text,
                             const std::string& value1,
                             const std::string& delimiter,
                             long long start, long long stop,
                             bool useKomma, long long* numValues )
{
    std::string line;
    while( std::getline( (*stream), line ) )
    {
        if( line.substr( 0, value1.length() ) == value1 )
        {
            (*numValues)++;
            if( (*numValues) >= start
                    && (*numValues) <= stop )
            {
                std::string value = line.substr( value1.length() );
                if( useKomma  )
                {
                    value = replacePointWithKomma( value );
                }
                (*text) += std::to_string( (*numValues) - start ) + delimiter
                        + value + "\r\n";
            }

            if( (*numValues) >= stop )
            {
                break;
            }
        }
    }
    _readWrite_.store( false );
}

void mainWindow::read2Values( std::ifstream* stream, std::string* text,
                              const std::string& value1,
                              const std::string& value2,
                              const std::string& delimiter,
                              long long start, long long stop,
                              bool useKomma, long long* numValues )
{
    std::string line, v1, v2;
    while( std::getline( (*stream), line ) )
    {
        if( line == "{" )
        {
            v1 = "";
            v2 = "";
        }
        if( line.substr( 0, value1.length() ) == value1 )
        {
            v1 = line.substr( value1.length() );
        }
        if( line.substr( 0, value2.length() ) == value2 )
        {
            v2 = line.substr( value2.length() );
        }
        if( line == "}" )
        {
            v1 = "";
            v2 = "";
        }

        if( v1 != ""
                && v2 != "" )
        {
            (*numValues)++;
            if( (*numValues) >= start
                    && (*numValues) <= stop )
            {
                if( useKomma )
                {
                    v1 = replacePointWithKomma( v1 );
                    v2 = replacePointWithKomma( v2 );
                }
                (*text) += v1 + delimiter + v2 + "\r\n";
            }

            if( (*numValues) >= stop )
            {
                break;
            }
        }
    }
    _readWrite_.store( false );
}

std::string mainWindow::replacePointWithKomma( std::string& text )
{
    while( text.find( '.' ) != std::string::npos )
    {
        text = text.replace( text.find( '.' ), 1, "," );
    }
    return text;
}

void mainWindow::writeTextToFile( const std::string &text,
                                  const std::string &fileName )
{
    std::ofstream outStream;
    outStream.open( fileName.c_str(), std::ios_base::out );

    if( !outStream.is_open() )
    {
        addInfoText( "Error: could not open output file!" );
        return;
    }

    addInfoText( "Writing output to file ..." );
    _readWrite_.store( true );
    std::thread writeThread( writeToFile, &outStream, text );
    while( _readWrite_.load() )
    {
        qApp->processEvents();
    }
    writeThread.join();
    outStream.close();
    addInfoText( "Values written to output file." );
}

void mainWindow::writeToFile( std::ofstream* stream,
                              const std::string &text )
{
    (*stream) << text;
    _readWrite_.store( false );
}

void mainWindow::clearInfoText()
{
    _ui->txt_info->clear();
}

void mainWindow::exit()
{
    qApp->exit();
}
