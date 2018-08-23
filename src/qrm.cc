#include "qrm.hh"
#include <QByteArray>
#include <QDebug>


QRMGenerator::QRMGenerator(QIODevice *source, size_t num, QObject *parent)
    : QIODevice(parent), _source(source), _enabled(false), _num(num), _encoder(_num),
      _generator(_num)
{
  if (_source) {
    connect(_source, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
    _source->open(QIODevice::ReadOnly);
  }

  for (size_t i=0; i<_num; i++) {
    double f = ((rand() % 700) + 300), wpm = 15 + (rand() % 10);
    MorseEncoder::Jitter jitter = MorseEncoder::Jitter(rand()%4);
		_encoder[i] = new MorseEncoder(f, f, wpm, wpm, MorseEncoder::SOUND_SHARP, jitter, this);
    _encoder[i]->open(QIODevice::ReadOnly);
		_generator[i] = new GenTextTutor(_encoder[i], ":/qso/qsogen.xml", this);
    connect(_generator[i], SIGNAL(sessionComplete()), _generator[i], SLOT(start()));
    _generator[i]->start();
	}
}

QRMGenerator::~QRMGenerator() {
  // pass...
}

bool
QRMGenerator::enabled() const {
  return _enabled;
}

void
QRMGenerator::enable(bool enabled) {
  _enabled = enabled;
}

int
QRMGenerator::stations() const {
  return _num;
}

void
QRMGenerator::setStations(int num) {
  // Delete old stations
  for (int i=0; i<_num; i++) {
    _generator[i]->deleteLater();
    _encoder[i]->deleteLater();
  }
  _generator.clear();; _encoder.clear();

  _num = num; _generator.resize(_num); _encoder.resize(_num);
  // Create new stations
  for (size_t i=0; i<_num; i++) {
    double f = ((rand() % 700) + 300), wpm = 15 + (rand() % 10);
    MorseEncoder::Jitter jitter = MorseEncoder::Jitter(rand()%4);
		_encoder[i] = new MorseEncoder(f, f, wpm, wpm, MorseEncoder::SOUND_SHARP, jitter, this);
    _encoder[i]->open(QIODevice::ReadOnly);
		_generator[i] = new GenTextTutor(_encoder[i], ":/qso/qsogen.xml", this);
    connect(_generator[i], SIGNAL(sessionComplete()), _generator[i], SLOT(start()));
    _generator[i]->start();
	}
}

void
QRMGenerator::setSource(QIODevice *src) {
  if (_source)
    disconnect(_source, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
  _source = src;
  if (! _source)
    return;
  connect(_source, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
  _source->open(QIODevice::ReadOnly);
}

qint64
QRMGenerator::bytesAvailable() const {
  if (! _source)
    return QIODevice::bytesAvailable();
  qint64 len = _source->bytesAvailable();
  if (_enabled)
    for (size_t i=0; i<_num; i++)
      len = std::min(len, _encoder[i]->bytesAvailable());
  return len + QIODevice::bytesAvailable();
}

qint64
QRMGenerator::readData(char *data, qint64 len) {
  if (! _source)
    return 0;

  if (! _enabled)
    return _source->read(data, len);


  len = std::min(len, _source->bytesAvailable());
	for (size_t i=0; i<_num; i++)
		len = std::min(len, _encoder[i]->bytesAvailable());
	if (0 == len)
		return 0;


	qint64 nsample = len/2;
	_source->read(data, 2*nsample);
	int16_t *out = (int16_t *)(data);
	for (qint64 i=0; i<nsample; i++) {
		double value = double(out[i])/2;
    int16_t qrmv=0;
		for (size_t j=0; j<_num; j++) {
      _encoder[j]->read((char *)&qrmv, 2);
      value += double(qrmv)/10/_num;
		}
		out[i] = value;
	}

  return 2*nsample;
}

qint64
QRMGenerator::writeData(const char *data, qint64 maxlen) {
  return 0;
}
