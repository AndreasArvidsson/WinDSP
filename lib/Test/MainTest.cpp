#include "SineSweepGenerator.h"

#include "BiquadFilter.h"
#include <algorithm>
#include "Convert.h"
#include <string>
#include <iostream>
#include <fstream>

class SeriesData {
public:
	std::string name;
	std::vector<std::vector<double>> data;
};

class GraphData {
public:
	GraphData(std::string n) {
		name = n;
	};
	std::string name;
	std::vector<SeriesData> series;
	void add(std::string name, BiquadFilter *pFilter) {
		SeriesData serie;
		serie.name = name;
		serie.data = pFilter->getFrequencyResponse(1000, 10, 20000);
		series.push_back(serie);
	}
};

void printMaxVal(BiquadFilter *pFilter) {
	SineSweepGenerator sine(pFilter->getSampleRate(), 10, 20000, 5);
	double maxVal = 0.0;
	double maxFreq;
	for (int i = 0; i < sine.getNumSamples(); ++i) {
		const double value = pFilter->process(sine.next());
		if (std::abs(value) > maxVal) {
			maxVal = std::abs(value);
			maxFreq = sine.getFrequency();
		}
	}
	printf("Max: %.2fHz, %.2fdB\n", maxFreq, Convert::levelToDb(maxVal));
}

std::string toString(double val) {
	std::string str = std::to_string(val);
	str.erase(str.find_last_not_of('0') + 1, std::string::npos);
	str.erase(str.find_last_not_of('.') + 1, std::string::npos);
	return str;
}

void addSeriesData(std::ofstream &stream, const SeriesData &serie, bool first) {
	stream << (first ? "" : ",\n");
	stream << "        {\n";
	stream << "            name: \"";
	stream << serie.name.c_str();
	stream << "\",\n";
	stream << "            data: [";
	first = true;
	for (const std::vector<double> &pair : serie.data) {
		stream << (first ? "" : ",");
		first = false;
		stream << "[" << pair[0] << ", " << pair[1] << "]";
	}
	stream << "]\n";
	stream << "        }";
}

void addGraphData(std::ofstream &stream, const GraphData *graph, bool first) {
	stream << (first ? "" : ",\n");
	stream << "{\n";
	stream << "    name: \"";
	stream << graph->name.c_str();
	stream << "\",\n";
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

void saveJsGraphData(std::vector<GraphData*> &graphs) {
	std::ofstream myfile;
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

void addCrossover(std::vector<GraphData*> &graphs, const uint32_t fs, const bool isLowPass, const double frequency, const CrossoverType type, std::vector<int> orders) {
	//"Highpass Bessel 100Hz",
	std::string name = (isLowPass ? "Lowpass " : "Highpass ");
	if (type == CrossoverType::Butterworth) { 
		name += "Butterworth ";
	} 
	else if (type == CrossoverType::Linkwitz_Riley) {
		name += "Linkwitz-Riley ";
	}
	else if (type == CrossoverType::Bessel) {
		name += "Bessel ";
	}
	name += toString(frequency) + "Hz";
	GraphData *graphData = new GraphData(name);
	for (int order : orders) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addCrossover(isLowPass, frequency, order, type);
		std::string name = toString(order);
		name += " order";
		graphData->add(name.c_str(), pFilter);
	}
	graphs.push_back(graphData);
}

void addShelf(std::vector<GraphData*> &graphs, const uint32_t fs, const bool isLowShelf, const double frequency, const double gain, std::vector<double> qValues) {
	std::string name = (isLowShelf ? "Lowshelf " : "Highshelf ") + toString(frequency) + "Hz, " + toString(gain) + "dB";
	GraphData *graphData = new GraphData(name);
	for (double q : qValues) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addShelf(isLowShelf, frequency, gain, q);
		std::string name = "Q ";
		name += toString(q);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

void addBandPass(std::vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, std::vector<double> bandwidths) {
	std::string name = "Bandpass " + toString(frequency) + "Hz, " + toString(gain) + "dB";
	GraphData *graphData = new GraphData(name);
	for (double bandwidth : bandwidths) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addBandPass(frequency, bandwidth, gain);
		std::string name = "BW ";
		name += toString(bandwidth);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

void addNotch(std::vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, std::vector<double> bandwidths) {
	std::string name = "Notch " + toString(frequency) + "Hz, " + toString(gain) + "dB";
	GraphData *graphData = new GraphData(name);
	for (double bandwidth : bandwidths) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addNotch(frequency, bandwidth, gain);
		std::string name = "BW ";
		name += toString(bandwidth);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

void addLT(std::vector<GraphData*> &graphs, const uint32_t fs, std::vector<double> qValues) {
	std::string name = "Linkwitz-Transform F0=30Hz, Q0=0.707, Fp=10Hz";
	GraphData *graphData = new GraphData(name);
	for (double q : qValues) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addLinkwitzTransform(30, 0.707, 10, q);
		std::string name = "Qp ";
		name += toString(q);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

void addPEQ(std::vector<GraphData*> &graphs, const uint32_t fs, const double frequency, const double gain, std::vector<double> qValues) {
	std::string name = "PEQ " + toString(frequency) + "Hz, " + toString(gain) + "dB";
	GraphData *graphData = new GraphData(name);
	for (double q : qValues) {
		BiquadFilter *pFilter = new BiquadFilter(fs);
		pFilter->addPEQ(frequency, q, gain);
		std::string name = "Q ";
		name += toString(q);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

int main(int argc, char **argv) {
	const uint32_t fs = 96000;

	std::vector<GraphData*> graphs;
	addCrossover(graphs, fs, true, 100, CrossoverType::Butterworth, { 1, 2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, false, 200, CrossoverType::Butterworth, { 1, 2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, true, 300, CrossoverType::Linkwitz_Riley, { 2, 4, 8 });
	addCrossover(graphs, fs, false, 400, CrossoverType::Linkwitz_Riley, { 2, 4, 8 });
	addCrossover(graphs, fs, true, 500, CrossoverType::Bessel, {  2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, false, 600, CrossoverType::Bessel, { 2, 3, 4, 5, 6, 7, 8 });
	addShelf(graphs, fs, true, 100, 10, {0.5, 0.707, 1, 2});
	addShelf(graphs, fs, false, 200, 3, { 0.5, 0.707, 1, 2 });
	addBandPass(graphs, fs, 500, -3, {0.25, 0.5, 1, 2, 4, 6, 8, 10});
	addNotch(graphs, fs, 500, 3, {0.25, 0.5, 1, 2, 4, 6, 8, 10});
	addPEQ(graphs, fs, 100, -6, {0.5, 1, 2, 5, 10});
	addPEQ(graphs, fs, 500, 3, { 0.5, 1, 2, 5, 10 });
	addLT(graphs, fs, { 0.25, 0.5, 0.707 });
	saveJsGraphData(graphs);
	
	BiquadFilter *pFilter = new BiquadFilter(fs);
	pFilter->addHighPass(100, 4, CrossoverType::Butterworth);
	printMaxVal(pFilter);
	pFilter->printCoefficients(true);

	//printf("\n");
	//system("PAUSE");
	//exit(EXIT_FAILURE);
	return 0;
}

//#include "SineSweepGenerator.h"
//SineSweepGenerator sine(44100, 2000, 10000, 5, 0);
//
//void CaptureLoop::_fillProcessBuffer() {
//	memset(_pProcessBuffer, 0, _renderBufferByteSize);
//	for (size_t sampleIndex = 0; sampleIndex < _renderBufferCapacity; ++sampleIndex) {
//		const double value = sine.next();
//		if (abs(value) > 1.0) {
//			printf("value clipping %f\n", value);
//		}
//		for (size_t channelIndex = 0; channelIndex < _nChannelsIn; ++channelIndex) {
//			(*_pInputs)[channelIndex]->route(value, _pProcessBuffer + sampleIndex * _nChannelsOut);
//		}
//	}
//}

