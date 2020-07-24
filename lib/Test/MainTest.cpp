#include "SineSweepGenerator.h"
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include "Convert.h"
#include "FilterBiquad.h"
#include "FilterCompression.h"
#include "CrossoverType.h"
#include "WaveHeader.h"
#include "File.h"
#include "Audioclient.h" //WAVE_FORMAT_PCM, WAVE_FORMAT_IEEE_FLOAT
#include "Error.h"

using std::ofstream;
using std::to_string;

class SeriesData {
public:
    string name;
    vector<vector<double>> data;
    vector<double> dataFlat;
};

class GraphData {
public:
    string name;
    bool isLog = true;
    vector<SeriesData> series;
    GraphData(string n) {
        name = n;
    };
    void add(string n, FilterBiquad *pFilter) {
        SeriesData serie;
        serie.name = n;
        serie.data = pFilter->getFrequencyResponse(1000, 10, 20000);
        series.push_back(serie);
    }
    void add(string n, vector<double> d) {
        SeriesData serie;
        serie.name = n;
        serie.dataFlat = d;
        series.push_back(serie);
    }
};

void printMaxVal(FilterBiquad *pFilter) {
    SineSweepGenerator sine(pFilter->getSampleRate(), 10, 20000, 5);
    double maxVal = 0;
    double maxFreq = 0;
    for (int i = 0; i < sine.getNumSamples(); ++i) {
        const double value = pFilter->process(sine.next());
        if (abs(value) > maxVal) {
            maxVal = abs(value);
            maxFreq = sine.getFrequency();
        }
    }
    printf("Max: %.2fHz, %.2fdB\n", maxFreq, Convert::levelToDb(maxVal));
}

string toString(double val) {
    string str = to_string(val);
    str.erase(str.find_last_not_of('0') + 1, string::npos);
    str.erase(str.find_last_not_of('.') + 1, string::npos);
    return str;
}

void addSeriesData(ofstream &stream, const SeriesData &serie, bool first) {
    stream << (first ? "" : ",\n");
    stream << "        {\n";
    stream << "            name: \"";
    stream << serie.name.c_str();
    stream << "\",\n";
    stream << "            data: [";
    first = true;
    for (const vector<double> &pair : serie.data) {
        stream << (first ? "" : ",");
        first = false;
        stream << "[" << pair[0] << ", " << pair[1] << "]";
    }
    for (const double value : serie.dataFlat) {
        stream << (first ? "" : ",");
        first = false;
        stream << value;
    }
    stream << "]\n";
    stream << "        }";
}

void addGraphData(ofstream &stream, const GraphData *graph, bool first) {
    stream << (first ? "" : ",\n");
    stream << "{\n";
    stream << "    name: \"";
    stream << graph->name.c_str();
    stream << "\",\n";
    stream << "    isLog: " << (graph->isLog ? "true" : "false") << ",\n";
    stream << "    series: [\n";
    first = true;
    for (const SeriesData &serie : graph->series) {
        addSeriesData(stream, serie, first);
        first = false;
    }
    stream << "\n";
    stream << "    ]\n";
    stream << "}";
}

void saveJsGraphData(vector<GraphData*> &graphs) {
    ofstream myfile;
    myfile.open("plotData.js");
    myfile << "var plotData = [\n";
    bool first = true;
    for (const GraphData *graph : graphs) {
        addGraphData(myfile, graph, first);
        first = false;
    }
    myfile << "\n];";
    myfile.close();
}

void addCrossover(vector<GraphData*> &graphs, const uint32_t fs, const bool isLowPass, const double frequency, const CrossoverType type, vector<uint8_t> orders) {
    //"Highpass Bessel 100Hz",
    string name = (isLowPass ? "Lowpass " : "Highpass ");
    if (type == CrossoverType::BUTTERWORTH) {
        name += "Butterworth ";
    }
    else if (type == CrossoverType::LINKWITZ_RILEY) {
        name += "Linkwitz-Riley ";
    }
    else if (type == CrossoverType::BESSEL) {
        name += "Bessel ";
    }
    name += toString(frequency) + "Hz";
    GraphData *graphData = new GraphData(name);
    for (uint8_t order : orders) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addCrossover(isLowPass, frequency, type, order);
        string n = toString(order);
        n += " order";
        graphData->add(n.c_str(), pFilter);
    }
    graphs.push_back(graphData);
}

void addShelf(vector<GraphData*> &graphs, const uint32_t fs, const bool isLowShelf, const double frequency, const double gain, vector<double> qValues) {
    string n = (isLowShelf ? "Lowshelf " : "Highshelf ") + toString(frequency) + "Hz, " + toString(gain) + "dB";
    GraphData *graphData = new GraphData(n);
    for (double q : qValues) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addShelf(isLowShelf, frequency, gain, q);
        n = "Q ";
        n += toString(q);
        graphData->add(n, pFilter);
    }
    graphs.push_back(graphData);
}

void addBandPass(vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, vector<double> bandwidths) {
    string n = "Bandpass " + toString(frequency) + "Hz, " + toString(gain) + "dB";
    GraphData *graphData = new GraphData(n);
    for (double bandwidth : bandwidths) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addBandPass(frequency, bandwidth, gain);
        n = "BW ";
        n += toString(bandwidth);
        graphData->add(n, pFilter);
    }
    graphs.push_back(graphData);
}

void addNotch(vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, vector<double> bandwidths) {
    string name = "Notch " + toString(frequency) + "Hz, " + toString(gain) + "dB";
    GraphData *graphData = new GraphData(name);
    for (double bandwidth : bandwidths) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addNotch(frequency, bandwidth, gain);
        string n = "BW ";
        n += toString(bandwidth);
        graphData->add(n, pFilter);
    }
    graphs.push_back(graphData);
}

void addLT(vector<GraphData*> &graphs, const uint32_t fs, vector<double> qValues) {
    string name = "Linkwitz-Transform F0=30Hz, Q0=0.707, Fp=10Hz";
    GraphData *graphData = new GraphData(name);
    for (double q : qValues) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addLinkwitzTransform(30, 0.707, 10, q);
        string n = "Qp ";
        n += toString(q);
        graphData->add(n, pFilter);
    }
    graphs.push_back(graphData);
}

void addPEQ(vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, vector<double> qValues) {
    string name = "PEQ " + toString(frequency) + "Hz, " + toString(gain) + "dB";
    GraphData *graphData = new GraphData(name);
    for (double q : qValues) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->addPEQ(frequency, gain, q);
        string n = "Q ";
        n += toString(q);
        graphData->add(n, pFilter);
    }
    graphs.push_back(graphData);
}

void addBiquad(vector<GraphData*> &graphs, const string name, const uint32_t fs, vector<vector<double>> biquad) {
    GraphData *graphData = new GraphData(name);
    FilterBiquad *pFilterSum = new FilterBiquad(fs);
    int stage = 1;
    for (const vector<double> &coeffs : biquad) {
        FilterBiquad *pFilter = new FilterBiquad(fs);
        pFilter->add(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]);
        pFilterSum->add(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]);
        graphData->add("Biquad " + toString(stage++), pFilter);
    }
    graphData->add("Sum", pFilterSum);
    graphs.push_back(graphData);
}

void compareLP(vector<GraphData*> &graphs, const uint32_t fs) {
    double freq = 80;
    GraphData *graphData = new GraphData("Compare LP");

    FilterBiquad *pFilter = new FilterBiquad(fs);
    pFilter->addLowPass(freq, CrossoverType::BUTTERWORTH, 5);
    graphData->add("BW", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addLowPass(freq, CrossoverType::BUTTERWORTH, 5, 0.2);
    graphData->add("BW Q+0.2", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addLowPass(freq, CrossoverType::BUTTERWORTH, 5, -0.2);
    graphData->add("BW Q-0.2", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addLowPass(freq, { -1, 0.5, 0.8 });
    graphData->add("Custom", pFilter);

    graphs.push_back(graphData);
}

void compareHP(vector<GraphData*> &graphs, const uint32_t fs) {
    double freq = 80;
    GraphData *graphData = new GraphData("Compare HP");

    FilterBiquad *pFilter = new FilterBiquad(fs);
    pFilter->addHighPass(freq, CrossoverType::BUTTERWORTH, 3);
    graphData->add("BW", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addHighPass(freq, CrossoverType::BUTTERWORTH, 3, 0.2);
    graphData->add("BW Q+0.2", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addHighPass(freq, CrossoverType::BUTTERWORTH, 3, -0.2);
    graphData->add("BW Q-0.2", pFilter);

    pFilter = new FilterBiquad(fs);
    pFilter->addHighPass(freq, { -1, 0.6 });
    graphData->add("Custom", pFilter);

    graphs.push_back(graphData);
}

void addCompression(vector<GraphData*>& graphs) {
    const File file("file_example_WAV_1MG.wav");

    std::unique_ptr<char[]> pBuffer;
    const size_t bufferSize = file.getData(&pBuffer);
    if (!bufferSize) {
        throw Error("Config - Can't read wav file. '%s'", file.getPath().c_str());
    }
    WaveHeader header;
    if (bufferSize < sizeof(header)) {
        throw Error("File is not a valid wav file. '%s'", file.getPath().c_str());
    }
    memcpy(&header, pBuffer.get(), sizeof(header));

    const size_t numSamples = header.getNumSamples();
    if (header.audioFormat != WAVE_FORMAT_PCM) {
        throw Error("Wav file is in unknown audio format: %u", header.audioFormat);
    }

    const char* pData = pBuffer.get() + sizeof(header);
    vector<double> samples;
    switch (header.bitsPerSample) {
    case 16:
        samples = Convert::pcm16ToDouble(pData, numSamples);
        break;
    case 24:
        samples = Convert::pcm24ToDouble(pData, numSamples);
        break;
    case 32:
        samples = Convert::pcm32ToDouble(pData, numSamples);
        break;
    default:
        throw Error("Wav file bit depth is unsupported: %u", header.bitsPerSample);
    }

    const double threshold = -15;
    const double ratio = 0.1;
    const double attack = 1;
    const double release = 1;
    const double window = 1;
    FilterCompression compWindow(header.sampleRate, threshold, ratio, attack, release, window);
    FilterCompression compSample(header.sampleRate, threshold, ratio, attack, release);
    vector<double> org, resWindow, resSample;
    //Only use first 5sec of first channel
    const size_t size = min(samples.size(), 5 * header.sampleRate);
    for (int i = 0; i < samples.size(); i += header.numChannels) {
        org.push_back(samples[i]);
        resWindow.push_back(compWindow.process(samples[i]));
        resSample.push_back(compSample.process(samples[i]));
    }

    GraphData* graphData = new GraphData("Compression 5sec");
    graphData->isLog = false;
    graphData->add("Org", org);
    graphData->add("No window", resSample);
    graphData->add("Window", resWindow);
    graphs.push_back(graphData);
}

int main() {
    const uint32_t fs = 96000;

    vector<GraphData*> graphs;
    addCrossover(graphs, fs, true, 100, CrossoverType::BUTTERWORTH, { 1, 2, 3, 4, 5, 6, 7, 8 });
    addCrossover(graphs, fs, false, 200, CrossoverType::BUTTERWORTH, { 1, 2, 3, 4, 5, 6, 7, 8 });
    addCrossover(graphs, fs, true, 300, CrossoverType::LINKWITZ_RILEY, { 2, 4, 8 });
    addCrossover(graphs, fs, false, 400, CrossoverType::LINKWITZ_RILEY, { 2, 4, 8 });
    addCrossover(graphs, fs, true, 500, CrossoverType::BESSEL, { 2, 3, 4, 5, 6, 7, 8 });
    addCrossover(graphs, fs, false, 600, CrossoverType::BESSEL, { 2, 3, 4, 5, 6, 7, 8 });
    addShelf(graphs, fs, true, 100, 10, { 0.5, 0.707, 1, 2 });
    addShelf(graphs, fs, false, 200, 3, { 0.5, 0.707, 1, 2 });
    addBandPass(graphs, fs, 500, -3, { 0.25, 0.5, 1, 2, 4, 6, 8, 10 });
    addNotch(graphs, fs, 500, 3, { 0.25, 0.5, 1, 2, 4, 6, 8, 10 });
    addPEQ(graphs, fs, 100, -6, { 0.5, 1, 2, 5, 10 });
    addPEQ(graphs, fs, 500, 3, { 0.5, 1, 2, 5, 10 });
    addLT(graphs, fs, { 0.25, 0.5, 0.707 });
    addBiquad(graphs, "Custom biquad 100Hz 4th order BW", fs, {
        { 0.993978831854144, -1.98795766370829,	0.993978831854144, -1.98793637410783, 0.987978953308752	},
        { 0.997490827915119, -1.99498165583024, 0.997490827915119, -1.99496029100786, 0.995003020652617 }
        });
    compareLP(graphs, fs);
    compareHP(graphs, fs);
    addCompression(graphs);
    saveJsGraphData(graphs);

    FilterBiquad *pFilter = new FilterBiquad(fs);
    pFilter->addHighPass(100, CrossoverType::BUTTERWORTH, 4);
    printMaxVal(pFilter);
    pFilter->printCoefficients(true);

    return 0;
}