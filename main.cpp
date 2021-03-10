#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;


#define CHUNK_SIZE 1024




//#define USEDEBUG
#define USEVERBOSE

#ifdef USEDEBUG
#define DebugNL( x ) cout << x << endl
#else
#define DebugNL( x )
#endif

#ifdef USEDEBUG
#define Debug( x ) cout << x
#else
#define Debug( x )
#endif

#ifdef USEVERBOSE
#define Verbose( x ) cout << x
#else
#define Verbose( x )
#endif

#ifdef USEVERBOSE
#define VerboseNL( x ) cout << x << endl
#else
#define VerboseNL( x )
#endif


enum Opcje {
	o_help,
	o_file_write,
	o_file_read,
	o_decode,
	o_encode,
	o_seed,
	o_info,
	o_notice
};

enum modes {
	m_help,
	m_encode,
	m_decode
};

enum readWrite{
	rw_read,
	rw_write
};

Opcje hashit(string const &input){
	if(input == "-h") return o_help;
	if(input == "-i") return o_file_read;
	if(input == "-o") return o_file_write;
	if(input == "-d") return o_decode;
	if(input == "-e") return o_encode;
	if(input == "-s") return o_seed;
	if(input == "-n") return o_notice;
}


void helpCommand(){
	cout << "Koder i enkoder na podstawie LFSR!" << endl << endl << "Dostępne argumenty: " << endl << "-h , wyświetla ten komunikat" << endl << "-e , zakoduj plik wejściowy" << endl << "-d , zdekoduj plik wejściowy" << endl << "-o <plik> , ustaw nazwę pliku wyjściowego" << endl << "-i <plik> , załaduj podany plik na wejście" << endl << "-s <0-65535> , podaj 'klucz' szyfrujacy" << endl ;
}



class files{
	friend class cipher;
	protected:

	string fileReadName;
	string fileWriteName;
	int readFileSize;
	int bytesLeftToRead;
	int bytesInBuffer;
	int bytesInfoRxBuffer;
	int bytesInfoTxBuffer;
	ifstream fileToRead;
	ofstream fileToWrite;
	char readFileChunk[CHUNK_SIZE];
	char processedOutputChunk[CHUNK_SIZE];
	char additionalInfoRead[CHUNK_SIZE];
	string additionalInfoWrite;
	char additionalInfoWriteChar[CHUNK_SIZE];
	public:
	files(){
		fileReadName = "";
		fileWriteName = "";
		readFileSize = 0;
		bytesLeftToRead = 0;
		bytesInBuffer = 0;
		bytesInfoTxBuffer = 0;
		bytesInfoRxBuffer = 0;

	}
	void setReadFileName(char in[]){
		fileReadName = in;
	}
	void setWriteFileName(char in[]){
		fileWriteName = in;
	}
	string getReadFileName(){
		return fileReadName;
	}
	string getWriteFileName(){
		return fileWriteName;
	}
	int bytesToRead(){
		if(bytesLeftToRead >= CHUNK_SIZE){
			return CHUNK_SIZE;
		} else {
			return bytesLeftToRead;
		}
	}
	int readHeader(){
	    #define SYNCWORD_LENGTH 9
	    #define SYNCWORD 7, 3, 7, 5, 2, 2, 4, 2, 7
	    #define SYNCWORD_PLUS_MESSAGE 7, 3, 7, 5, 2, 2, 4, 2, 8
	    char syncWord[SYNCWORD_LENGTH] = {SYNCWORD};
	    char syncWordMessage[SYNCWORD_LENGTH] = {SYNCWORD_PLUS_MESSAGE};
	    char readSync[SYNCWORD_LENGTH];
	    fileToRead.read(readSync, SYNCWORD_LENGTH);
	    int mode = 1;
	    for(int i=0; i<SYNCWORD_LENGTH; i++){
            if(readSync[i] != syncWord[i]){ mode = 0; }
	    }
	    if(mode == 1){
            DebugNL("READ SYNC WORD");
            bytesLeftToRead -= SYNCWORD_LENGTH;
            return 1;
	    }
        mode = 1;
        for(int i=0; i<SYNCWORD_LENGTH; i++){
            if(readSync[i] != syncWordMessage[i]){ mode = 0; }
	    }
	    if(mode == 1){
	        DebugNL("READ SYNC WORD + MSG");
	        char length[1];
            fileToRead.read(length, 1);
            bytesInfoRxBuffer = length[0];
            fileToRead.read(additionalInfoRead, bytesInfoRxBuffer);
            bytesLeftToRead -= bytesInfoRxBuffer + SYNCWORD_LENGTH + 1;
            return 2;
	    }
	    return 0;
	}
	void showInfo(){
	    Verbose("Message attached to file: ");
	    if(bytesInfoRxBuffer == 0){
            VerboseNL("None");
            return;
	    }
        for(int i=0; i<bytesInfoRxBuffer; i++){
            Verbose((char)additionalInfoRead[i]);

        }
        VerboseNL("");
        VerboseNL("Length of info " << bytesInfoRxBuffer);
	}
	int openReadFile(){
		fileToRead.open(fileReadName, ios::binary);
		if(fileToRead.is_open()){
			VerboseNL("Otwarto plik " << fileReadName);

		} else {
			VerboseNL("Nie otwarto pliku!" << fileReadName);
			return 0;
		}
		readFileSize = fileToRead.tellg();
		fileToRead.seekg(0, ios::end);
		readFileSize = fileToRead.tellg() - readFileSize;
		fileToRead.seekg(0, ios::beg);
		bytesLeftToRead = readFileSize;

	}
	int openWriteFile(){
		fileToWrite.open(fileWriteName, ios::binary);
		if(fileToWrite.is_open()){
			VerboseNL("Otwarto plik " << fileWriteName);

		} else {
			VerboseNL("Nie otwarto pliku!" << fileWriteName);
			return 0;
		}

	}
	void closeReadFile(){
		fileToRead.close();
	}
	void closeWriteFile(){
		fileToWrite.close();
	}
	int checkReadFileSize(){
        //cout << "sprawdzono rozmiar - " << readFileSize << endl;
		return readFileSize;
	}
	int loadFileChunk(){
		int toRead = bytesToRead();
		if(toRead>0){
			fileToRead.read(readFileChunk, toRead);
			bytesLeftToRead -= toRead;
			bytesInBuffer = toRead;
			return 1;
		}
		return 0;
	}
    void addHeader(){
        char syncWord[SYNCWORD_LENGTH] = {SYNCWORD};
	    char syncWordMessage[SYNCWORD_LENGTH] = {SYNCWORD_PLUS_MESSAGE};
        if(bytesInfoTxBuffer == 0){
            fileToWrite.write(syncWord,SYNCWORD_LENGTH);
        } else {
            fileToWrite.write(syncWordMessage,SYNCWORD_LENGTH);
            char toWrite[1];
            toWrite[0] = bytesInfoTxBuffer;
            //toWrite.append(bytesInfoTxBuffer[0]);
            //string toWrite = "";
            bytesInfoTxBuffer = additionalInfoWrite.length();
            fileToWrite.write(toWrite,1);

            strcpy(additionalInfoWriteChar, additionalInfoWrite.c_str());
            fileToWrite.write(additionalInfoWriteChar,bytesInfoTxBuffer);
        }

    }
    void setMessage(char in[]){
        additionalInfoWrite = in;
        bytesInfoTxBuffer = additionalInfoWrite.length();
    }
	int saveSelectedBytes(){
		//processedOutputChunk[CHUNK_SIZE]
		DebugNL("saving bytes -> ");
		fileToWrite.write(processedOutputChunk,bytesInBuffer);
	}


}files0;



class cipher{
	protected:
	uint16_t init_seed;
	int numberOfGeneratedSequence;
	uint8_t lfsr_sequence[ CHUNK_SIZE ];
	public:
	cipher(){
		init_seed = 0;
		numberOfGeneratedSequence = 0;
	}
	void generateNewSequence(int length){ // generates CHUNK_SIZE Bytes of sequence
		numberOfGeneratedSequence++;
		int state;
		lfsr_sequence[0] = init_seed & (0b0000000011111111);

		lfsr_sequence[1] = ((init_seed & (0b1111111100000000)) >> 8);
		for(int byte=3; byte<length; byte=byte+2){
			show16bitWord(init_seed);
			state = 0;
			for(int bit=0; bit<16; bit++){
				int state[5] = {0,0,0,0,0};
				if( init_seed & 0b1000000000000000 ){ state[0] = 1; };
				if( init_seed & 0b0010000000000000 ){ state[1] = 1; };
				if( init_seed & 0b0001000000000000 ){ state[2] = 1; };
				if( init_seed & 0b0000010000000000 ){ state[3] = 1; };
				state[4] = (((state[0]^state[1])^state[2])^state[3]);
				init_seed = init_seed << 1;
				if(state[4]){
					init_seed = init_seed | 1;
				} else {
					init_seed = init_seed & (0b1111111111111110);
				}

			}
			lfsr_sequence[byte] = init_seed & (0b0000000011111111);
			lfsr_sequence[byte+1] = ((init_seed & (0b1111111100000000)) >> 8);
		}
	}
	void show16bitWord(uint16_t &in){
		for(int i=0; i<16; i++){

			if((in & (1<<15-i))>0){
				Debug("1");
			} else {
				Debug("0");
			}

		}
		Debug(", ");
	}
	int setSeed(uint16_t &in){
		if( (in > 0) && (in < 65535) && (numberOfGeneratedSequence == 0) ){
			init_seed = in;
			VerboseNL("Successfully set seed " << init_seed);
			return 0;
		} else {
			VerboseNL("Error occured during setting seed!");
			DebugNL("numberOfGeneratedSequence = " << numberOfGeneratedSequence << ", init_seed = " << init_seed);
			return -1;
		}
	}
	void showSequence(){
		for(int i=0; i<CHUNK_SIZE; i++){
			Debug( (char)lfsr_sequence[i] << ", ");
		}
		DebugNL("");
	}
	void processChunk(files &in){
		DebugNL( "processing chunk -> ");
		int length = in.bytesInBuffer;
		generateNewSequence(length);
		for(int i=0; i<length; i++){
			in.processedOutputChunk[i] = in.readFileChunk[i]^lfsr_sequence[i];
		}
	}
	void resetCipher(){
		init_seed = 0;
		numberOfGeneratedSequence = 0;
	}
	friend class work;
}cipher0;

class work{
	int mode; // 0 means help,  1 means encode, 2 means decode
	int seed;
	public:
	work(){
		mode = -1;
	}
	void setHelp(){
		mode = m_help;
	}
	void setEncode(){
		mode = m_encode;
	}
	void setDecode(){
		mode = m_decode;
	}
	int getSeed(){
		return seed;
	}
	int checkMode(){
		return mode;
	}

}work0;




int main(int argc, char *argv[]){



	if(argc > 1){
		int indexOfArguments = 0;
		while(indexOfArguments < argc){
            Debug("indexOfArguments = " << indexOfArguments);
			string arg = argv[indexOfArguments];
			Debug( "Teraz przetwarzam " << arg );
			switch(hashit(arg)) {
				case o_help:{
					work0.setHelp();
					indexOfArguments++;
				}
				break;
				case o_file_read:{
					files0.setReadFileName(argv[indexOfArguments+1]);
					Debug("Read from " << argv[indexOfArguments+1]);
					indexOfArguments = indexOfArguments + 2;

				}
				break;
				case o_file_write:{
					files0.setWriteFileName(argv[indexOfArguments+1]);
					Debug("Write to " << argv[indexOfArguments+1]);
					indexOfArguments = indexOfArguments + 2;

				}
				break;
				case o_seed:{
					uint16_t seed = 0;
					//scanf(argv[indexOfArguments+1], "%d", &seed);
                    seed =strtoul(argv[indexOfArguments+1], NULL, 0);
					cipher0.setSeed(seed);
					indexOfArguments = indexOfArguments + 2;
                    VerboseNL("SEED = " << (uint16_t)seed);
				}
				break;
				case o_decode:{
					work0.setDecode();
					indexOfArguments++;
					Debug("Decoding choosen");
				}
				break;
				case o_encode:{
					work0.setEncode();
					indexOfArguments++;
					Debug("Encoding choosen");
				}
				break;
				case o_notice:{
					files0.setMessage(argv[indexOfArguments+1]);
					Debug("Notice  " << argv[indexOfArguments+1] << " is added.");
					indexOfArguments = indexOfArguments + 2;
					Debug("Notice to add");

				}
				break;
				default:{
					VerboseNL("Jeden z argumentów nie został rozpoznany! Użyj komendy -h by wyświetlić dostępne argumenty!");
					return 0;
				}
			}
		}
	} else {
		VerboseNL("Nie podano argumentów! Użyj komendy -h by wyświetlić dostępne argumenty!");
		return 0;
	}
	if(work0.checkMode() == m_help){
		helpCommand();
		return 0;
	}
	if(work0.checkMode() == m_encode){
		files0.openReadFile();
		files0.openWriteFile();
		//cipher0.showSequence();
		VerboseNL("Rozmiar pliku do zakodowania " << files0.getReadFileName() << " wynosi " << files0.checkReadFileSize() << " Bajty.");


		files0.addHeader();
		while(files0.loadFileChunk()){
			cipher0.processChunk(files0);
			files0.saveSelectedBytes();
		}

		files0.closeReadFile();
		files0.closeWriteFile();

	}
	if(work0.checkMode() == m_decode){
		files0.openReadFile();
		files0.openWriteFile();
        VerboseNL("Rozmiar pliku do zdekodowania " << files0.getReadFileName() << " wynosi " << files0.checkReadFileSize() << " Bajty.");

		//cipher0.showSequence();
		int isHeader = files0.readHeader();
		VerboseNL("header " << isHeader);
        if(isHeader>0){
            files0.showInfo();
        } else {
            VerboseNL("Podano nieprawidłowy plik!");
            return 0;
        }

		while(files0.loadFileChunk()){
			cipher0.processChunk(files0);
			files0.saveSelectedBytes();
		}

		files0.closeReadFile();
		files0.closeWriteFile();

	}
	//Zakończono ładowanie ustawianie parametrów programu


	//cout << "Seed = " << seed << endl;

	// Otwarcie Pliku

	return 0;

}
