#include "SineSweepGenerator.h"
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include "Convert.h"
#include "CrossoverTypes.h"
#include "FilterBiquad.h"

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
	void add(std::string name, FilterBiquad *pFilter) {
		SeriesData serie;
		serie.name = name;
		serie.data = pFilter->getFrequencyResponse(1000, 10, 20000);
		series.push_back(serie);
	}
};

void printMaxVal(FilterBiquad *pFilter) {
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
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
		FilterBiquad *pFilter = new FilterBiquad(fs);
		pFilter->addPEQ(frequency, q, gain);
		std::string name = "Q ";
		name += toString(q);
		graphData->add(name, pFilter);
	}
	graphs.push_back(graphData);
}

void addBiquad(std::vector<GraphData*> &graphs, const std::string name, const uint32_t fs, std::vector<std::vector<double>> biquad) {
	GraphData *graphData = new GraphData(name);
	FilterBiquad *pFilterSum = new FilterBiquad(fs);
	int stage = 1;
	for (const std::vector<double> &coeffs : biquad) {
		FilterBiquad *pFilter = new FilterBiquad(fs);
		pFilter->add(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]);
		pFilterSum->add(coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]);
		graphData->add(toString(stage++), pFilter);
	}
	graphData->add("Sum", pFilterSum);
	graphs.push_back(graphData);
}

void compareLP(std::vector<GraphData*> &graphs, const uint32_t fs) {
	double freq = 80;
	GraphData *graphData = new GraphData("Compare LP");

	FilterBiquad *pFilter = new FilterBiquad(fs);
	pFilter->addLowPass(freq, 5, CrossoverType::Butterworth);
	graphData->add("BW", pFilter);

	pFilter = new FilterBiquad(fs);
	pFilter->addLowPass(freq, { -1, 0.54118411083, 1.21578947368 });
	graphData->add("Custom", pFilter);

	graphs.push_back(graphData);
}

int main(int argc, char **argv) {
	const uint32_t fs = 96000;

	std::vector<GraphData*> graphs;
	addCrossover(graphs, fs, true, 100, CrossoverType::Butterworth, { 1, 2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, false, 200, CrossoverType::Butterworth, { 1, 2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, true, 300, CrossoverType::Linkwitz_Riley, { 2, 4, 8 });
	addCrossover(graphs, fs, false, 400, CrossoverType::Linkwitz_Riley, { 2, 4, 8 });
	addCrossover(graphs, fs, true, 500, CrossoverType::Bessel, { 2, 3, 4, 5, 6, 7, 8 });
	addCrossover(graphs, fs, false, 600, CrossoverType::Bessel, { 2, 3, 4, 5, 6, 7, 8 });
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
	saveJsGraphData(graphs);

	FilterBiquad *pFilter = new FilterBiquad(fs);
	pFilter->addHighPass(100, 4, CrossoverType::Butterworth);
	printMaxVal(pFilter);
	pFilter->printCoefficients(true);

	return 0;
}