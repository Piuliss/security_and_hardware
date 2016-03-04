#include <stdint.h>
#include <stdlib.h>

typedef unsigned char BYTE;

#define NUM_E 2.718281
#define W 32
#define R 20
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//static variables
int * S;
double goldenRatio = 1.6180339887496482;

int Pw=0xb7e15163, Qw=0x9e3779b9; // magic values


int * convBytesWords(BYTE * key, int u, int c) {
	int tmp[c];
	for (int i = 0; i < sizeof(tmp); i++)
		tmp[i] = 0;

	for (int i = 0, off = 0; i < c; i++)
		tmp[i] = ((key[off++] & 0xFF)) | ((key[off++] & 0xFF) << 8)
				| ((key[off++] & 0xFF) << 16) | ((key[off++] & 0xFF) << 24);

	return tmp;
}
int * generateSubkeys(BYTE * key) {
		int key_length = sizeof(key) /sizeof(BYTE);
		int u = W / 8;
		int c = key_length / u;
		int t = 2 * R + 4;

		int * L = convBytesWords(key, u, c);


		int S[t];
		S[0] = Pw;
		for (int i = 1; i < t; i++)
			S[i] = S[i - 1] + Qw;

		int A = 0;
		int B = 0;
		int k = 0, j = 0;

		int v = 3 * MAX(c, t);

		for (int i = 0; i < v; i++) {
			A = S[k] = rotl((S[k] + A + B), 3);
			B = L[j] = rotl(L[j] + A + B, A + B);
			k = (k + 1) % t;
			j = (j + 1) % c;

		}

		return S;
	}

	int rotl(int a, int n) {
		n &= 0x1f; /* higher rotates would not bring anything */
		return ( (a<<n)| (a>>(32-n)) );
	}
	int rotr(int a, int n) {
		n &= 0x1f; /* higher rotates would not bring anything */
		return ( (a>>n)| (a<<(32-n)) );
	}

BYTE * decryptBloc(BYTE * input){
	int input_length = sizeof(input) /sizeof(BYTE);
	BYTE tmp[input_length];
	int t,u;
	int aux;
	int data[input_length/4];
	for(int i =0;i<input_length;i++)
		data[i] = 0;
	int off = 0;
	for(int i=0;i<input_length;i++){
		data[i] = 	((input[off++]&0xff))|
					((input[off++]&0xff) << 8) |
					((input[off++]&0xff) << 16) |
					((input[off++]&0xff) << 24);
	}


	int A = data[0],B = data[1],C = data[2],D = data[3];

	C = C - S[2*R+3];
	A = A - S[2*R+2];
	for(int i = R;i>=1;i--){
		aux = D;
		D = C;
		C = B;
		B = A;
		A = aux;

		u = rotl(D*(2*D+1),5);
		t = rotl(B*(2*B + 1),5);
		C = rotr(C-S[2*i + 1],t) ^ u;
		A = rotr(A-S[2*i],u) ^ t;
	}
	D = D - S[1];
	B = B - S[0];

	data[0] = A;data[1] = B;data[2] = C;data[3] = D;

	int tmp_length = sizeof(tmp)/sizeof(BYTE);
	for(int i = 0;i< tmp_length;i++){
		tmp[i] = (BYTE)((data[i/4] >> (i%4)*8) & 0xff);
	}

	return tmp;
}
BYTE * encrypBloc(BYTE * input, int length){
	BYTE  tmp [length];
	int t, u;
	int aux;
	int data[length];
    for(int i =0;i<length;i++)
         data[i] = 0;
    int off = 0;
    for(int i=0;i<length;i++){
         data[i] = ((input[off++]&0xff))|
           ((input[off++]&0xff) << 8)|
           ((input[off++]&0xff) << 16)|
           ((input[off++]&0xff) << 24);
    }
    int A = data[0],B = data[1],C = data[2],D = data[3];
    B = B + S[0];
    D = D + S[1];
    for(int i = 1;i<=R;i++){
         t = rotl(B*(2*B+1),5);
         u = rotl(D*(2*D+1),5);
         A = rotl(A^t,u)+S[2*i];
         C = rotl(C^u,t)+S[2*i+1];

         aux = A;
         A = B;
         B = C;
         C = D;
         D = aux;
    }

    return 0;
}
BYTE * paddingKey(BYTE * key) {
	int key_length = sizeof(key)/sizeof(BYTE);
	int l = key_length%4;
	BYTE newkey[key_length+l];;
	memcpy(newkey, key, key_length);
	for (int i = 0; i < l; i++){
		newkey[key_length+i] = 0;
	}
	return newkey;
}

BYTE * deletePadding(BYTE * input){
	int count = 0;
	int input_length= sizeof(input) / sizeof(BYTE);
	int i = input_length - 1;
	while (input[i] == 0) {
		count++;
		i--;
	}
	int tmp_length = input_length - count - 1;
	BYTE tmp[tmp_length];
	memcpy(tmp, input, tmp_length);
	return tmp;
}
BYTE * encrypt(BYTE * data, BYTE * key) {
	BYTE  bloc[16];
	key = paddingKey(key);
	S = generateSubkeys(key);
	int length_data = sizeof(data) / sizeof(BYTE);
	int lenght = 16 - length_data % 16;
	BYTE padding[lenght];
	padding[0] = (BYTE) 0x80;

	for (int i = 1; i < lenght; i++)
		padding[i] = 0;
	int count = 0;
	BYTE tmp[length_data+lenght];
	//afiseazaMatrice(S);
	int i;
	for(i=0;i<length_data+lenght;i++){
		if(i>0 && i%16 == 0){
			BYTE * bloc2 = encryptBloc(bloc);
			int bloc_length = sizeof(bloc2) / sizeof(BYTE);
			for(int k =0; k < bloc_length; k++){
				tmp[i-16 + k] = bloc2[k];
			}
		}

		if (i < length_data)
			bloc[i % 16] = data[i];
		else{
			bloc[i % 16] = padding[count];
			count++;
			if(count>lenght-1) count = 1;
		}
	}
	BYTE * bloc2 = encryptBloc(bloc);
	int bloc_length = sizeof(bloc2) / sizeof(BYTE);
	for(int k =0; k < bloc_length; k++){
		tmp[i-16 + k] = bloc2[k];
	}
	return tmp;
}
BYTE * decrypt(BYTE * data, BYTE * key) {
	int data_length = sizeof(data) / sizeof(data);
	BYTE tmp[data_length];
	BYTE bloc[16];
	key = paddingKey(key);
	S = generateSubkeys(key);
	int i;
	for(i=0;i<data_length;i++){
		if(i>0 && i%16 == 0){
			BYTE * bloc2 = decryptBloc(bloc);
			int bloc_length = sizeof(bloc2) / sizeof(BYTE);
			for(int k =0; k < bloc_length; k++){
				tmp[i-16 + k] = bloc2[k];
			}
		}

		if (i < data_length)
			bloc[i % 16] = data[i];
	}

	BYTE * bloc2 = decryptBloc(bloc);
	int bloc_length = sizeof(bloc2) / sizeof(BYTE);
	for(int k =0; k < bloc_length; k++){
		tmp[i-16 + k] = bloc2[k];
	}

	BYTE * tmp2 = deletePadding(tmp);
	return tmp2;
}

int main (){
	BYTE * data = "Hola Mundo";
	BYTE * key = "key";
	BYTE * data_enc = encrypt(data,key);
	printf("Data Encrypted %lld", data_enc );

}
