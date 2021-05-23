//#include <iostream>
//#include <cmath>
//#include <iomanip>
//using namespace std;
//typedef unsigned char uc;
//
//string matrixInSbox[8] = { "10001111", "11000111", "11100011", "11110001", "11111000", "01111100", "00111110","00011111" };
//
//
//uc bitlen(unsigned short num) {
//	uc i;
//	for (i = 0; i <= 8; i++)
//	{
//		if (!(num >> (i + 1)))
//			return i;
//	}
//}
//
//uc divide(unsigned short a, uc b, uc & r) {
//	uc a_len = bitlen(a);
//	uc b_len = bitlen(b);
//
//	if (a < b) {
//		r = a;
//		return 0;
//	}
//
//	uc diff = a_len - b_len;
//	unsigned short temp = b;
//	temp = temp << diff;
//	a = a ^ temp;
//
//	return (1 << diff) | divide(a, b, r);
//}
//
//uc multiply(uc a, uc b) {
//	uc res = 0;
//	if (b & 0x01)
//		res = a;
//	for (int i = 1; i < 8; i++) {
//		if (b & (0x01 << i)) {
//			uc temp = a;
//			for (int j = 0; j < i; j++) {
//				if (!(temp & 0x80))
//					temp <<= 1;
//				else {
//					temp <<= 1;
//					temp = temp ^ 0x1B;
//				}
//			}
//			res = res ^ temp;
//		}
//	}
//	return res;
//}
//
//uc inverse(uc b) {
//	if (b == 0)
//		return 0;
//
//	short r0 = 0x011B;
//	uc r1 = b, r2, q;
//	uc v0 = 1, w0 = 0;
//	uc v1 = 0, w1 = 1;
//	uc v2, w2;
//
//	q = divide(r0, r1, r2);
//	v2 = v0 ^ multiply(q, v1);	//v2=1
//	w2 = w0 ^ multiply(q, w1);	//w2=q                                        
//
//	while (1) {
//		/*cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(q) << " ";
//		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(v2) << " ";
//		cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(w2) << "\n";*/
//		if (r2 == 0)
//			break;
//		r0 = r1;
//		r1 = r2;
//		q = divide(r0, r1, r2);
//		v0 = v1;
//		v1 = v2;
//		v2 = v0 ^ multiply(q, v1);
//		w0 = w1;
//		w1 = w2;
//		w2 = w0 ^ multiply(q, w1);
//	}
//
//	cout << hex << uppercase << setfill('0') << setw(2) << static_cast<int>(w1) << "\n";
//	//cout << dec;
//	//cout << static_cast<int>(w1);
//
//	return w1;
//}
//
//string toBin(uc hex) {
//	int num = static_cast<int>(hex);
//	string bin_str = "00000000";
//
//	for (int i = 0; i < 8; i++)
//	{
//		if (num % 2 == 0)
//			bin_str[i] = '0';
//		else
//			bin_str[i] = '1';
//		num = num / 2;
//	}
//	return bin_str;
//}
//
//uc toUnsignedChar(string bin) {
//	int temp = 0;
//	for (int i = 0; i < 8; i++)
//	{
//		if (bin[i] == '1')
//			temp += pow(2, i);
//	}
//
//	uc res = static_cast<uc>(temp);
//	return res;
//}
//uc s_box(uc u) {
//	// 1) °ö¼ÀÀÇ ¿ª¿ø
//	uc inv = inverse(u);
//
//	/* Çà·Ä°ö */
//	// char to bin
//	string bin_str = toBin(inv);
//	string res = "00000000";	// Çà·Ä°ö °á°ú¸¦ ÀúÀåÇÏ´Â res
//
//	for (int j = 0; j < 8; j++) {
//		int sum = 0;
//		for (int i = 0; i < 8; i++) {
//			if ((matrixInSbox[j][i] & bin_str[i]) == '1')
//				sum++;
//		}
//		if (sum % 2 == 1)
//			res[j] = '1';
//	}
//
//	cout << "µ¡¼À Àü: " << res << "\n";
//
//	/*¿­º¤ÅÍ µ¡¼À*/
//	uc vec = 63;
//	uc ret = toUnsignedChar(res);
//	cout << hex << uppercase << setfill('0') << setw(2) << "µ¡¼À Àü: " << static_cast<int>(ret) << "\n";
//
//	ret = vec ^ ret;
//
//	cout << hex << uppercase << setfill('0') << setw(2) << "µ¡¼À ÈÄ: " << static_cast<int>(ret) << "\n";
//	cout << "µ¡¼À ÈÄ: " << toBin(ret) << "\n";
//	return ret;
//}
//
//
//int main() {
//	s_box(0);
//}