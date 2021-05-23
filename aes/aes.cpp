#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
using namespace std;
typedef unsigned char uc;

uc key[16];	
uc roundKey[16 * 11];	// ���� Ű�� ����Ʈ ������ ����
						// ex) round[0] ~ round[16]�� 0 ���� Ű

// S-box�� ��İ��� ���Ǵ� ���
string matrixInSbox[8] = {"10001111", "11000111", "11100011", "11110001", "11111000", "01111100", "00111110","00011111"};
// S-box�� ��� RCj
uc RC[11] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 231, 41 };

// ���ڷ� �־����� short�� 2���������� ��Ʈ�� ��ȯ�ϴ� �Լ�
uc bitlen(unsigned short num) {
	uc i;
	for (i = 0; i <= 8; i++)
	{
		if (!(num >> (i + 1)))
			return i;
	}
}

// ������ ������ ���ϱ� ���� binary ������ ����
uc divide(unsigned short a, uc b, uc& r) {
	uc a_len = bitlen(a);
	uc b_len = bitlen(b);

	if (a < b) {
		r = a;
		return 0;
	}

	uc diff = a_len - b_len;
	unsigned short temp = b;
	temp = temp << diff;
	a = a ^ temp;

	return (1 << diff) | divide(a, b, r);
}


// ������ ������ ���ϱ� ���� binary ���� ����
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
				else {
					temp <<= 1;
					temp = temp ^ 0x1B;
				}
			}
			res = res ^ temp;
		}
	}
	return res;
}

// ������ ���� ���ϴ� �Լ� inverse
uc inverse(uc b) {
	if (b == 0)
		return 0;

	short r0 = 0x01E7;
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

// unsigned char ������ 2���� string ���·� ��ȯ
string toBin(uc hex) {
	int num = static_cast<int>(hex);
	string bin_str = "00000000";

	for (int i = 0; i < 8; i++)
	{
		if (num % 2 == 0)
			bin_str[i] = '0';	// ��İ� �� MSB, LSB ������ �ݴ��̱� ������
		else
			bin_str[i] = '1';
		num = num / 2;
	}
	return bin_str;
}

// 2���� string ������ ������ unsigned char ������ ��ȯ
uc toUnsignedChar(string bin) {
	int temp = 0;
	for (int i = 0; i < 8; i++)
	{
		if (bin[i] == '1')
			temp += pow(2, i);
	}

	uc res = static_cast<uc>(temp);
	return res;
}

// ����Ʈ ������ s_box ���� ���ִ� �Լ�
uc s_box(int idx) {
	/* 1) ������ ���� */
	uc inv = inverse(roundKey[idx]);

	/* 2) ��İ� */
	string bin_str = toBin(inv); // inv�� 2���� string ���·� �����ϴ� ���� bin_str
	string res = "00000000";	// ��İ� ����� 2���� string ���·� �����ϴ� ���� res

	for (int j = 0; j < 8; j++) {
		int sum = 0;
		for (int i = 0; i < 8; i++) {
			if ((matrixInSbox[j][i] & bin_str[i]) == '1')
				sum++;
		}
		if (sum % 2 == 1)
			res[j] = '1';
	}

	/* 3) ������ ���� */
	uc vec = 99;	// vec="01100011"
	uc ret = toUnsignedChar(res);	// ��İ� ��� res�� unsigned char ���·� ret�� ����
	ret = vec ^ ret;
	
	return ret;
}

/* Key expansion */
void keyExpansion() {
	// 0 ���� Ű�� ó�� Ű �״��
	for (size_t i = 0; i < 16; i++){
		roundKey[i] = key[i];
	}

	for (int i = 1; i < 11; i++)
	{
		/* g function */
		// ���� ����Ű�� 4��° ���尡���ͼ� g�Լ� ���
		// permutation
		roundKey[16 * i] = roundKey[16 * i - 3];
		roundKey[16 * i + 1] = roundKey[16 * i - 2];
		roundKey[16 * i + 2] = roundKey[16 * i - 1];
		roundKey[16 * i + 3] = roundKey[16 * i - 4];

		// ����Ʈ���� S-box
		roundKey[16 * i] = s_box(16 * i);
		roundKey[16 * i + 1] = s_box(16 * i + 1);
		roundKey[16 * i + 2] = s_box(16 * i + 2);
		roundKey[16 * i + 3] = s_box(16 * i + 3);

		// RCj 0 0 0�� xor
		roundKey[16 * i] = RC[i] ^ roundKey[16 * i];
		/* end of g function */

		//���� ���� xor
		// 1��° ���� ����
		for (int j = 0; j < 4; j++){
			roundKey[16 * i + j] = roundKey[16 * (i - 1) + j] ^ roundKey[16 * i + j];
		}
		// 2, 3, 4��° ���� ����
		for (int j = 4; j < 16; j++)
		{
			roundKey[16 * i + j] = roundKey[16 * i + j - 4] ^ roundKey[16 * (i - 1) + j];
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

	// key ���
	for (size_t i = 0; i < 16; i++)
		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(key[i]) << " ";
	cout << "\n";

	keyExpansion();

	// output expanded key
	cout << "expanded key: \n";
	for (int i = 0; i < 11; i++)
	{
		cout << dec << "k" << i << ": ";
		for (int j = 0; j < 16; j++)
		{	
			cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(roundKey[16 * i + j]) << " ";
		}
		cout << "\n";
	}

	ifstream plainFile("plain.bin", ios::binary);

	if (!plainFile.is_open()) {
		cout << "plain.bin ������ ã�� �� �����ϴ�\n";
		return 0;
	}

	plainFile.seekg(0, ios::end);
	int size = plainFile.tellg();
	cout << dec <<"\nplain.bin's size = " <<size << "\n";

	plainFile.clear();
	plainFile.seekg(0, ios::beg);

	uc* plain = new uc[size];
	//uc plain[48];
	plainFile.read((char*)plain, size);

	for (size_t i = 0; i < size; i++){
		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(plain[i]) << " ";
		if((i+1) % 16 ==0)
			cout << "\n";
	}

	return 0;
}