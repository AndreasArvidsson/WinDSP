/*
	This file contains the handling of Operating system error messages.
	Validation and plain text descriptions.

	Author: Andreas Arvidsson
	Source: https://github.com/AndreasArvidsson/WinDSP
*/

#pragma once
#include <string>
#include "Error.h"
#include <Audioclient.h> //HRESULT

namespace ErrorMessages {

	//Convert HRESULT to text explanation.
    const std::string hresult(const HRESULT hr);
    const std::string getLastError();

};

inline void assert(const HRESULT hr) {
	if (FAILED(hr)) {
		throw Error("WASAPI (0x%08x) %s", hr, ErrorMessages::hresult(hr).c_str());
	}
}

