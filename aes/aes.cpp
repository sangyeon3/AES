#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
using namespace std;
typedef unsigned char uc;

uc key[16];				// 16바이트 키를 저장
uc roundKey[16 * 11];	// 11개 라운드 키를 바이트 단위로 저장

uc plain[16];			// 16바이트 plaintext 블록을 저장
uc cipher[16];			// 16바이트 ciphertext 블록을 저장

string matrixInSbox[8] = {"10001111", "11000111", "11100011", "11110001", "11111000", "01111100", "00111110","00011111"};	// S-box에서 행렬곱에 사용되는 행렬(행 기준)
uc RC[11] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 231, 41 };	// S-box의 상수 RCj (10진수)
uc matrixInMixColumns[16] = { 2, 3, 1, 1, 1, 2, 3, 1, 1, 1, 2, 3, 3, 1, 1, 2 };		// mix columns의 행렬곱에 사용되는 행렬 (10진수)

// inverse 버전 변수
string inv_matrixInSbox[8] = { "00100101", "10010010", "01001001", "10100100", "01010010", "00101001", "10010100", "01001010" };
uc inv_matrixInMixColumns[16] = { 14, 11, 13, 9, 9, 14, 11, 13, 13, 9, 14, 11, 11, 13, 9, 14 };

uc bitlen(unsigned short num);
uc divide(unsigned short a, uc b, uc& r);
uc multiply(uc a, uc b);
uc inverse(uc b);
string toBin(uc hex);
uc toUnsignedChar(string bin);
uc s_box(int idx);
void keyExpansion();
void encryption();
uc inverseS_box(uc input);
void decryption();

// 인자 num의 2진수에서의 비트 수 반환하는 함수
uc bitlen(unsigned short num) {
	uc i;
	for (i = 0; i <= 8; i++) {
		if (!(num >> (i + 1)))
			return i;
	}
}

// 2진수 a / b 나눗셈하여 몫을 리턴해주는 함수
// 나머지는 인자 r에 저장
uc divide(unsigned short a, uc b, uc& r) {
	uc a_len = bitlen(a);
	uc b_len = bitlen(b);
	if (a < b) {
		r = a;
		return 0;
	}

	uc diff = a_len - b_len;
	unsigned short temp = b;
	temp = temp << diff;		// a, b의 비트 수 차이만큼 b를 left shift하여 a와 MSB 위치 맞춰주고
	a = a ^ temp;				// a에서 shift 연산한 b(=temp)를  빼준다

	// a < b가 될 때까지 나눗셈 연산 반복
	return (1 << diff) | divide(a, b, r);		
}


// a * b 2진수 곱셈하여 결과를 리턴해주는 함수
// overhead 발생 시 irreducible polynomial로 modulo reduction 수행
uc multiply(uc a, uc b) {
	uc res = 0;
	// b의 LSB = 1이면 res=a
	if (b & 0x01)
		res = a;

	// b의 각 비트가 1이면 a를 해당 비트 위치에 맞추어 더해줌
	for (int i = 1; i < 8; i++) {
		if (b & (0x01 << i)) {		// b의 각 비트가 1이면
			uc temp = a;
			// 1비트씩 a를 left shift 하는 것을 총 i번 반복
			for (int j = 0; j < i; j++) {
				// overhead 발생하지 않으면 1비트 left shift 
				if (!(temp & 0x80))
					temp <<= 1;
				// overhead 발생하면 modulo reduction
				else {
					temp <<= 1;
					temp = temp ^ 0xE7;
				}
			}
			res = res ^ temp;	// 비트 1이 되는 b의 비트 자릿수에 맞춘 a를 결과에 res에 더해줌
		}
	}
	return res;
}

// 확장된 유클리드 이용하여 인자 b의 irreducible polynomial에 대한 곱셈의 역원 리턴해주는 함수
uc inverse(uc b) {
	if (b == 0)
		return 0;

	short r0 = 0x01E7;		// irreducible polynomial
	// 확장 유클리드에 사용되는 두 다항식 설정
	uc r1 = b, r2, q;
	uc v0 = 1, w0 = 0;
	uc v1 = 0, w1 = 1;
	uc v2, w2;
	
	q = divide(r0, r1, r2);
	v2 = v0 ^ multiply(q, v1);	
	w2 = w0 ^ multiply(q, w1);	                                       

	while (1) {
		if (r2 == 0)
			break;
		r0 = r1; 
		r1 = r2;
		q = divide(r0, r1, r2);
		v0 = v1;
		v1 = v2;
		v2 = v0 ^ multiply(q, v1);
		w0 = w1;
		w1 = w2;
		w2 = w0 ^ multiply(q, w1);
	}
	return w1;
}

// unsigned char 변수를 8비트 2진수 string으로 리턴해주는 함수
string toBin(uc hex) {
	int num = static_cast<int>(hex);
	string bin_str = "00000000";

	for (int i = 0; i < 8; i++) {
		if (num % 2 == 0)
			bin_str[i] = '0';	// 행렬곱 시 MSB, LSB 순서가 반대이기 때문에
		else
			bin_str[i] = '1';
		num = num / 2;
	}
	return bin_str;
}

// 8비트 2진수 string을 unsigned char 형으로 리턴해주는 함수
uc toUnsignedChar(string bin) {
	int temp = 0;
	for (int i = 0; i < 8; i++) {
		if (bin[i] == '1')
			temp += pow(2, i);
	}

	uc res = static_cast<uc>(temp);
	return res;
}

// s_box 연산한 결과 리턴해주는 함수 (바이트 단위)
uc s_box(uc input) {
	/* 1) 곱셈의 역원 */
	uc inv = inverse(input);

	/* 2) 행렬곱 */
	string bin_inv = toBin(inv);		// inv를 2진수 string 형태로 저장하는 변수 bin_inv
	string res = "00000000";			// 행렬곱 결과를 2진수 string 형태로 저장하는 변수 res

	for (int j = 0; j < 8; j++) {		// 행
		int sum = 0;
		for (int i = 0; i < 8; i++) {	// 비트
			if ((matrixInSbox[j][i] & bin_inv[i]) == '1')
				sum++;
		}
		if (sum % 2 == 1)
			res[j] = '1';
	}

	/* 3) 열벡터 덧셈 */
	uc vec = 99;						// "01100011"을 의미
	uc ret = toUnsignedChar(res);
	ret = vec ^ ret;
	
	return ret;
}

// key를 roundKey[0] ~ [16*11]로 key expansion 해주는 함수
void keyExpansion() {
	// 0 라운드 키는 처음 키 그대로
	for (size_t i = 0; i < 16; i++){
		roundKey[i] = key[i];
	}

	// 1~10 라운드 키
	for (int i = 1; i < 11; i++)
	{
		/* g function */
		// permutation
		roundKey[16 * i] = roundKey[16 * i - 3];
		roundKey[16 * i + 1] = roundKey[16 * i - 2];
		roundKey[16 * i + 2] = roundKey[16 * i - 1];
		roundKey[16 * i + 3] = roundKey[16 * i - 4];

		// 바이트별로 S-box 통과
		for (int j = 0; j < 4; j++)
			roundKey[16 * i + j] = s_box(roundKey[16 * i + j]);

		// RCj 0 0 0과 xor
		roundKey[16 * i] = RC[i] ^ roundKey[16 * i];
		/* end of g function */

		/* 워드 단위 xor */
		// 1번째 워드 연산
		for (int j = 0; j < 4; j++){
			roundKey[16 * i + j] = roundKey[16 * (i - 1) + j] ^ roundKey[16 * i + j];
		}
		// 2, 3, 4번째 워드 연산
		for (int j = 4; j < 16; j++) {
			roundKey[16 * i + j] = roundKey[16 * i + j - 4] ^ roundKey[16 * (i - 1) + j];
		}
	}
}

// 16바이트 블록 plain을 encryption하여 plain에 저장
void encryption() {
	// print starting state
	cout << "\nStarting encryption state: \n";
	for (size_t i = 0; i < 16; i++) {
		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(plain[i]) << " ";
	}

	// Round 0: add round key
	// ㅁㅁㅁ평문과 0번 라운드 키 xor
	for (int i = 0; i < 16; i++) {
		plain[i] = plain[i] ^ roundKey[i];
	}

	cout << "\nAR: ";
	for (size_t i = 0; i < 16; i++) {
		cout << static_cast<int>(plain[i]) << " ";
	}

	// Round 1~10
	// round 10은 mix columns 연산 X
	for (int r = 1; r < 11; r++) {

		cout << dec << "\nRound " << r;

		// 1) Substitute bytes
		for (int j = 0; j < 16; j++)
			plain[j] = s_box(plain[j]);

		cout << "\nBS: ";
		for (size_t i = 0; i < 16; i++) {
			cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(plain[i]) << " ";
		}

		// 2) Shift rows
		// 2행
		uc temp = plain[1];
		plain[1] = plain[5]; plain[5] = plain[9]; plain[9] = plain[13]; plain[13] = temp;
		// 3행
		temp = plain[2];
		plain[2] = plain[10]; plain[10] = temp;
		temp = plain[6];
		plain[6] = plain[14]; plain[14] = temp;
		// 4행
		temp = plain[3];
		plain[3] = plain[15]; plain[15] = plain[11]; plain[11] = plain[7]; plain[7] = temp;

		cout << "\nSR: ";
		for (size_t i = 0; i < 16; i++) {
			cout << static_cast<int>(plain[i]) << " ";
		}

		// 3) Mix columns
		uc temp_byte[4];
		if (r != 10) {
			for (int k = 0; k < 4; k++) {
				for (int i = 0; i < 4; i++) {		// 행
					temp = 0;
					for (int j = 0; j < 4; j++) {	// 열
						temp = temp ^ multiply(matrixInMixColumns[(4 * i) + j], plain[4 * k + j]);
					}
					temp_byte[i] = temp;
				}
				for (int i = 0; i < 4; i++) {
					plain[(4 * k) + i] = temp_byte[i];
				}
			}

			cout << "\nMC: ";
			for (size_t i = 0; i < 16; i++) {
				cout << static_cast<int>(plain[i]) << " ";
			}
		}

		// 4) Add round key
		for (int i = 0; i < 16; i++) {
			plain[i] = plain[i] ^ roundKey[16 * r + i];
		}

		cout << "\nAR: ";
		for (size_t i = 0; i < 16; i++) {
			cout << static_cast<int>(plain[i]) << " ";
		}
	}
}

// 바이트 단위로 s_box 연산 해주는 함수 (inverse.ver)
uc inverseS_box(uc input) {

	/* 1) 행렬곱 */
	string bin_inv = toBin(input);
	string res = "00000000";

	for (int j = 0; j < 8; j++) {
		int sum = 0;
		for (int i = 0; i < 8; i++) {
			if ((inv_matrixInSbox[j][i] & bin_inv[i]) == '1')
				sum++;
		}
		if (sum % 2 == 1)
			res[j] = '1';
	}

	/* 2) 열벡터 덧셈 */
	uc vec = 5;						// vec는 "00000101"를 의미
	uc ret = toUnsignedChar(res);	
	ret = vec ^ ret;

	/* 3) 곱셈의 역원 */
	ret = inverse(ret);

	return ret;
}

void decryption() {
	// print starting state
	cout << "\nStarting decryption state: \n";
	for (size_t i = 0; i < 16; i++) {
		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(cipher[i]) << " ";
	}

	// Round 0: add round key
	// ciphertext와 10 라운드 키 xor
	for (int i = 0; i < 16; i++) {
		cipher[i] = cipher[i] ^ roundKey[16*10 + i];
	}

	cout << "\nAR: ";
	for (size_t i = 0; i < 16; i++) {
		cout << static_cast<int>(cipher[i]) << " ";
	}

	// Round 1~10, round 10은 mix columns 연산 X
	for (int r = 1; r < 11; r++) {

		cout << dec << "\nRound " << r;

		// 1) Shift rows
		// 2행
		uc temp = cipher[13];
		cipher[13] = cipher[9]; cipher[9] = cipher[5]; cipher[5] = cipher[1]; cipher[1] = temp;
		// 3행
		temp = cipher[2];
		cipher[2] = cipher[10]; cipher[10] = temp;
		temp = cipher[6];
		cipher[6] = cipher[14]; cipher[14] = temp;
		// 4행
		temp = cipher[3];
		cipher[3] = cipher[7]; cipher[7] = cipher[11]; cipher[11] = cipher[15]; cipher[15] = temp;

		cout << "\nSR: ";
		for (size_t i = 0; i < 16; i++) {
			cout << hex << static_cast<int>(cipher[i]) << " ";
		}

		// 2) Substitute bytes
		for (int j = 0; j < 16; j++)
			cipher[j] = inverseS_box(cipher[j]);

		cout << "\nBS: ";
		for (size_t i = 0; i < 16; i++) {
			cout << static_cast<int>(cipher[i]) << " ";
		}

		// 3) Add round key
		for (int i = 0; i < 16; i++) {
			cipher[i] = cipher[i] ^ roundKey[(16 * 10) - (16 * r - i)];
		}

		cout << "\nAR: ";
		for (size_t i = 0; i < 16; i++) {
			cout << static_cast<int>(cipher[i]) << " ";
		}

		// 4) Mix columns
		uc temp_byte[4];
		if (r != 10) {
			for (int k = 0; k < 4; k++) {
				for (int i = 0; i < 4; i++) {
					temp = 0;
					for (int j = 0; j < 4; j++) {
						temp = temp ^ multiply(inv_matrixInMixColumns[(4 * i) + j], cipher[4 * k + j]);
					}
					temp_byte[i] = temp;
				}
				for (int i = 0; i < 4; i++) {
					cipher[(4 * k) + i] = temp_byte[i];
				}
			}

			cout << "\nMC: ";
			for (size_t i = 0; i < 16; i++) {
				cout << static_cast<int>(cipher[i]) << " ";
			}
		}
	}
}


int main() {
	/* key expansion */
	ifstream keyFile("key.bin", ios::binary);

	if (!keyFile.is_open()) {
		cout << "key.bin 파일을 찾을 수 없습니다\n";
		return 0;
	}

	keyFile.read((char*)& key, 16);
	keyFile.close();

	// key 출력
	for (size_t i = 0; i < 16; i++)
		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(key[i]) << " ";

	keyExpansion();

	// print expanded key
	cout << "\nexpanded key: \n";
	for (int i = 0; i < 11; i++) {
		cout << dec << "k" << i << ": ";
		for (int j = 0; j < 16; j++) {	
			cout << hex << static_cast<int>(roundKey[16 * i + j]) << " ";
		}
		cout << "\n";
	}

	// select encryption or decryption
	char mode;
	cout << "\ninput e or d: ";
	cin >> mode;

	/* encryption */
	if (mode == 'e') {

		ifstream plainFile("plain.bin", ios::binary);
		if (!plainFile.is_open()) {
			cout << "plain.bin 파일을 찾을 수 없습니다\n";
			return 0;
		}

		ofstream cipherFile("cipher.bin", ios::binary);
		if (!cipherFile.is_open()) {
			cout << "cipher.bin 파일을 찾을 수 없습니다\n";
			return 0;
		}

		// encryption by byte
		while (plainFile.read((char*)& plain, 16)) {
			encryption();
			cipherFile.write((char*)& plain, 16);
		}
		cout << "\n\nEncryption complete!\n";

		plainFile.close();
		cipherFile.close();
	}
	/* decryption */
	else if (mode == 'd') {

		ifstream cipherFile("cipher.bin", ios::binary);
		if (!cipherFile.is_open()) {
			cout << "cipher.bin 파일을 찾을 수 없습니다\n";
			return 0;
		}

		ofstream plainFile2("plain2.bin", ios::binary);
		if (!plainFile2.is_open()) {
			cout << "plain2.bin 파일을 찾을 수 없습니다\n";
			return 0;
		}

		// decryption by byte
		while (cipherFile.read((char*)& cipher, 16)) {
			decryption();
			plainFile2.write((char*)& cipher, 16);
		}
		cout << "\n\nDecryption complete!\n";

		cipherFile.close();
		plainFile2.close();
	}
	else
		cout << "잘못된 입력입니다\n";

	return 0;
}