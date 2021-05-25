#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
using namespace std;
typedef unsigned char uc;

uc key[16];				// 16����Ʈ Ű�� ����
uc roundKey[16 * 11];	// 11�� ���� Ű�� ����Ʈ ������ ����

uc plain[16];			// 16����Ʈ plaintext ����� ����
uc cipher[16];			// 16����Ʈ ciphertext ����� ����

// S-box���� ��İ��� ���Ǵ� ���(�� ����)
string matrixInSbox[8] = {"10001111", "11000111", "11100011", "11110001", "11111000", "01111100", "00111110","00011111"};
uc RC[11] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 231, 41 };	// S-box�� ��� RCj (10����)
uc matrixInMixColumns[16] = { 2, 3, 1, 1, 1, 2, 3, 1, 1, 1, 2, 3, 3, 1, 1, 2 };		// mix columns�� ��İ��� ���Ǵ� ��� (10����)

// inverse ���� ����
string inv_matrixInSbox[8] = { "00100101", "10010010", "01001001", "10100100", "01010010", "00101001", "10010100", "01001010" };
uc inv_matrixInMixColumns[16] = { 14, 11, 13, 9, 9, 14, 11, 13, 13, 9, 14, 11, 11, 13, 9, 14 };

int bitlen(unsigned short num);
uc divide(unsigned short a, uc b, uc& r);
uc multiply(uc a, uc b);
uc inverse(uc b);
string toBin(uc hex);
uc toUnsignedChar(string bin);
uc s_box(uc input);
void keyExpansion();
void encryption();
uc inverseS_box(uc input);
void decryption();

// ���� num�� 2���������� ��Ʈ �� ��ȯ�ϴ� �Լ�
int bitlen(unsigned short num) {
	for (int i = 0; i <= 8; i++) {
		if (!(num >> (i + 1)))
			return i;
	}
}

// 2���� a / b �������Ͽ� ���� �������ִ� �Լ�. �������� ���� r�� ����
uc divide(unsigned short a, uc b, uc& r) {
	
	if (a < b) {
		r = a;
		return 0;
	}
	int a_len = bitlen(a);
	int b_len = bitlen(b);

	uc diff = a_len - b_len;
	unsigned short temp = b;
	temp = temp << diff;
	a = a ^ temp;

	return (1 << diff) | divide(a, b, r);		
}


// a * b 2���� �����Ͽ� ����� �������ִ� �Լ�
uc multiply(uc a, uc b) {
	uc res = 0;
	if (b & 0x01)
		res = a;

	for (int i = 1; i < 8; i++) {
		if (b & (0x01 << i)) {
			uc temp = a;
			for (int j = 0; j < i; j++) {
				if (!(temp & 0x80))
					temp <<= 1;
				// overhead �߻��ϸ� modulo reduction
				else {
					temp <<= 1;
					temp = temp ^ 0xE7;
				}
			}
			res = res ^ temp;
		}
	}
	return res;
}

// Ȯ��� ��Ŭ���� �̿��Ͽ� ���� b�� irreducible polynomial�� ���� ������ ���� �������ִ� �Լ�
uc inverse(uc b) {
	if (b == 0)
		return 0;

	// Ȯ�� ��Ŭ���忡 ���Ǵ� �� ���׽� ����
	uc v0 = 1, w0 = 0;
	uc v1 = 0, w1 = 1;
	short r0 = 0x01E7;		// irreducible polynomial x^8 + x^7 + x^6 + x^5 + x^2 + x + 1
	uc r1 = b, r2;
	uc q, v2, w2;
	
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

// unsigned char ������ 8��Ʈ 2���� string���� �������ִ� �Լ�
string toBin(uc hex) {
	int num = static_cast<int>(hex);
	string bin_str = "00000000";

	for (int i = 0; i < 8; i++) {
		if (num % 2 == 0)
			bin_str[i] = '0';			// ��İ� �� MSB, LSB ������ �ݴ��̱� ������
		else
			bin_str[i] = '1';
		num = num / 2;
	}
	return bin_str;
}

// 8��Ʈ 2���� string�� unsigned char ������ �������ִ� �Լ�
uc toUnsignedChar(string bin) {
	int temp = 0;
	for (int i = 0; i < 8; i++) {
		if (bin[i] == '1')
			temp += pow(2, i);
	}

	uc res = static_cast<uc>(temp);
	return res;
}

// s_box ������ ��� �������ִ� �Լ� (����Ʈ ����)
uc s_box(uc input) {
	/* 1) ������ ���� */
	uc inv = inverse(input);

	/* 2) ��İ� */
	string bin_inv = toBin(inv);		// inv�� 2���� string ���·� �����ϴ� ���� bin_inv
	string res = "00000000";			// ��İ� ����� 2���� string ���·� �����ϴ� ���� res

	for (int j = 0; j < 8; j++) {
		int sum = 0;
		for (int i = 0; i < 8; i++) {
			if ((matrixInSbox[j][i] & bin_inv[i]) == '1')
				sum++;
		}
		if (sum % 2 == 1)
			res[j] = '1';
	}

	/* 3) ������ ���� */
	uc vec = 99;						// "01100011"�� �ǹ�
	uc ret = toUnsignedChar(res);
	ret = vec ^ ret;
	
	return ret;
}

// key�� roundKey[0] ~ [16*11]�� key expansion ���ִ� �Լ�
void keyExpansion() {
	// 0 ���� Ű�� ó�� Ű �״��
	for (size_t i = 0; i < 16; i++){
		roundKey[i] = key[i];
	}

	// 1~10 ���� Ű
	for (int i = 1; i < 11; i++){
		/* g function */
		// permutation
		roundKey[16 * i] = roundKey[16 * i - 3];
		roundKey[16 * i + 1] = roundKey[16 * i - 2];
		roundKey[16 * i + 2] = roundKey[16 * i - 1];
		roundKey[16 * i + 3] = roundKey[16 * i - 4];

		// ����Ʈ���� S-box ���
		for (int j = 0; j < 4; j++)
			roundKey[16 * i + j] = s_box(roundKey[16 * i + j]);

		// RCj 0 0 0�� xor
		roundKey[16 * i] = RC[i] ^ roundKey[16 * i];
		/* end of g function */

		/* ���� ���� xor */
		// 1�� ���� ����
		for (int j = 0; j < 4; j++){
			roundKey[16 * i + j] = roundKey[16 * (i - 1) + j] ^ roundKey[16 * i + j];
		}
		// 2, 3, 4�� ���� ����
		for (int j = 4; j < 16; j++) {
			roundKey[16 * i + j] = roundKey[16 * i + j - 4] ^ roundKey[16 * (i - 1) + j];
		}
	}
}

// 16����Ʈ ��� plain�� encryption�Ͽ� plain�� ����
void encryption() {

	// Round 0: add round key
	for (int i = 0; i < 16; i++) {
		plain[i] = plain[i] ^ roundKey[i];
	}

	// Round 1~10. round 10�� mix columns ���� X
	for (int r = 1; r < 11; r++) {

		// 1) Substitute bytes
		for (int j = 0; j < 16; j++)
			plain[j] = s_box(plain[j]);

		// 2) Shift rows
		// 2��
		uc temp = plain[1];
		plain[1] = plain[5]; plain[5] = plain[9]; plain[9] = plain[13]; plain[13] = temp;
		// 3��
		temp = plain[2];
		plain[2] = plain[10]; plain[10] = temp;
		temp = plain[6];
		plain[6] = plain[14]; plain[14] = temp;
		// 4��
		temp = plain[3];
		plain[3] = plain[15]; plain[15] = plain[11]; plain[11] = plain[7]; plain[7] = temp;

		// 3) Mix columns
		uc temp_byte[4];
		if (r != 10) {
			for (int k = 0; k < 4; k++) {
				for (int i = 0; i < 4; i++) {
					temp = 0;
					for (int j = 0; j < 4; j++) {
						temp = temp ^ multiply(matrixInMixColumns[(4 * i) + j], plain[4 * k + j]);
					}
					temp_byte[i] = temp;
				}
				for (int i = 0; i < 4; i++) {
					plain[(4 * k) + i] = temp_byte[i];
				}
			}
		}

		// 4) Add round key
		for (int i = 0; i < 16; i++) {
			plain[i] = plain[i] ^ roundKey[16 * r + i];
		}
	}
}

// ����Ʈ ������ s_box ���� ���ִ� �Լ� (inverse.ver)
uc inverseS_box(uc input) {

	/* 1) ��İ� */
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

	/* 2) ������ ���� */
	uc vec = 5;						// vec�� "00000101"�� �ǹ�
	uc ret = toUnsignedChar(res);	
	ret = vec ^ ret;

	/* 3) ������ ���� */
	ret = inverse(ret);

	return ret;
}

void decryption() {

	// Round 0: add round key
	for (int i = 0; i < 16; i++) {
		cipher[i] = cipher[i] ^ roundKey[16*10 + i];
	}

	// Round 1~10, round 10�� mix columns ���� X
	for (int r = 1; r < 11; r++) {

		// 1) Shift rows
		// 2��
		uc temp = cipher[13];
		cipher[13] = cipher[9]; cipher[9] = cipher[5]; cipher[5] = cipher[1]; cipher[1] = temp;
		// 3��
		temp = cipher[2];
		cipher[2] = cipher[10]; cipher[10] = temp;
		temp = cipher[6];
		cipher[6] = cipher[14]; cipher[14] = temp;
		// 4��
		temp = cipher[3];
		cipher[3] = cipher[7]; cipher[7] = cipher[11]; cipher[11] = cipher[15]; cipher[15] = temp;

		// 2) Substitute bytes
		for (int j = 0; j < 16; j++)
			cipher[j] = inverseS_box(cipher[j]);

		// 3) Add round key
		for (int i = 0; i < 16; i++) {
			cipher[i] = cipher[i] ^ roundKey[(16 * 10) - (16 * r - i)];
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
		}
	}
}


int main() {
	/* key expansion */
	ifstream keyFile("key.bin", ios::binary);

	if (!keyFile.is_open()) {
		cout << "key.bin ������ ã�� �� �����ϴ�\n";
		return 0;
	}

	keyFile.read((char*)& key, 16);
	keyFile.close();

	keyExpansion();

	// select encryption or decryption
	char mode;
	cout << "input e or d: ";
	cin >> mode;

	/* encryption */
	if (mode == 'e') {

		ifstream plainFile("plain.bin", ios::binary);
		if (!plainFile.is_open()) {
			cout << "plain.bin ������ ã�� �� �����ϴ�\n";
			return 0;
		}

		ofstream cipherFile("cipher.bin", ios::binary);
		if (!cipherFile.is_open()) {
			cout << "cipher.bin ������ ã�� �� �����ϴ�\n";
			return 0;
		}

		// encryption by byte
		while (plainFile.read((char*)& plain, 16)) {
			encryption();
			cipherFile.write((char*)& plain, 16);
		}
		cout << "\nEncryption complete!\n";

		plainFile.close();
		cipherFile.close();
	}
	/* decryption */
	else if (mode == 'd') {

		ifstream cipherFile("cipher.bin", ios::binary);
		if (!cipherFile.is_open()) {
			cout << "cipher.bin ������ ã�� �� �����ϴ�\n";
			return 0;
		}

		ofstream plainFile2("plain2.bin", ios::binary);
		if (!plainFile2.is_open()) {
			cout << "plain2.bin ������ ã�� �� �����ϴ�\n";
			return 0;
		}

		// decryption by byte
		while (cipherFile.read((char*)& cipher, 16)) {
			decryption();
			plainFile2.write((char*)& cipher, 16);
		}
		cout << "\nDecryption complete!\n";

		cipherFile.close();
		plainFile2.close();
	}
	else
		cout << "�߸��� �Է��Դϴ�\n";

	return 0;
}