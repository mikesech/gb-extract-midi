#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <array>
#include <utility>
#include <type_traits>

class LoadError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

template<typename CharType>
void read(std::istream& stream, CharType&& buffer, std::streamsize count) {
	stream.read(std::forward<CharType>(buffer), count);
	if(!stream.good())
		throw LoadError("stream went bad");
}

class ChunkID : public std::array<char, 4> {
public:
	using std::array<char, 4>::array;

	/*bool operator==(const char* x) const {
		// FIXME: Doesn't respect null character ending x
		return std::memcmp(data(), x, size()) == 0;
	}
	bool operator!=(const char* x) const {
		return !(*this == x);
	}*/
	template <unsigned N>
	bool operator==(const char (&x)[N]) const {
		static_assert(N == 5, "chunk ID must be size 4"); // 4 + 1 because of null terminator
		return std::memcmp(data(), x, size()) == 0;
	}
	template <unsigned N>
	bool operator!=(const char (&x)[N]) const {
		return !(*this == x);
	}
	std::string toString() const {
		return {data(), size()};
	}
	static ChunkID deserialize(std::istream& stream) {
		ChunkID x;
		read(stream, x.data(), x.size());
		return x;
	}
};
// At -O3, this will get unrolled and converted into the
// __builtin_bswap32,64 primitives.
template <typename T>
T fromBigEndian(const T& t) {
    const unsigned char* const x = reinterpret_cast<const unsigned char*>(&t);
    T output = 0;
    for(int i = 0; i < sizeof(T); ++i)
        output |= x[i] << (8 * (sizeof(T) - i - 1));
    return output;
}
/*
// At -O3, this will get unrolled and converted into the
// __builtin_bswap32,64 primitives.
template <typename T>
T bitswap(const T& t) {
    T x = 0;
    for(int i = 0; i < sizeof(T); ++i) {
        const unsigned char byte = (t >> (8 * i)) & 0xffu;
        x |= byte << (8 * (sizeof(T) - i - 1));
    }
    return x;
}
template <typename T>
typename std::enable_if<sizeof(T) == 4, T>::type bitswap(const T& t) {
	return __builtin_bswap32(t);
}
template <typename T>
typename std::enable_if<sizeof(T) == 8, T>::type bitswap(const T& t) {
	return __builtin_bswap64(t);
}
*/
template <typename T>
T deserializeInteger(std::istream& stream) {
	T buffer;
	read(stream, reinterpret_cast<char*>(&buffer), sizeof(T));
	return fromBigEndian(buffer);
}
std::string loadBytes(std::istream& stream, size_t length) {
	std::string x(length, '\0');
	read(stream, &x[0], length);
	return x;
}

void assertMagicNumber(std::istream& stream) {
	ChunkID id = ChunkID::deserialize(stream);
	if(id != "FORM")
		throw LoadError("missing FORM chunk");
}
void assertAiffChunk(std::istream& stream) {
	ChunkID id = ChunkID::deserialize(stream);
	if(id != "AIFF")
		throw LoadError("missing AIFF chunk");
}

int loadHeader(std::istream& stream) {
	assertMagicNumber(stream);
	const int chunkSize = deserializeInteger<uint32_t>(stream);
	assertAiffChunk(stream);
	return chunkSize;
}



int main(int argc, char* argv[]) try {
	if(argc != 2) {
		std::cerr << "invalid invocation\n";
		return 1;
	}
	const std::string path = argv[1];
	std::ifstream file(path);
	if(!file.is_open()) {
		std::cerr << "cannot open file\n";
		return 2;
	}

	const int size = loadHeader(file);
	std::cerr << "Loaded AIFF file\n";
	// Skip over non MIDI chunks
	while(true) {
		ChunkID id = ChunkID::deserialize(file);
		if(id == ".mid") {
			break;
		} else {
			std::cerr << "Skipping over non-MIDI chunk (" << id.toString() << ", ";
			uint32_t chunkSize = deserializeInteger<uint32_t>(file);
			std::cerr << chunkSize << " bytes)" << std::endl;
			file.ignore(chunkSize);
		}
	}

	// Process MIDI chunk
	int chunkSize = deserializeInteger<uint32_t>(file);
	std::cerr << "Found .mid chunk (" << chunkSize << " bytes)\n";
	std::string midiChunk = loadBytes(file, chunkSize);
	std::cout << midiChunk;

	return 0;
} catch(LoadError& e) {
	std::cerr << "Error loading input file: " << e.what() << std::endl;
}
