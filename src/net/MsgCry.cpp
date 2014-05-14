#include "MsgCry.h"



#define ENCRYPT_SIZE		0x0060
#define ALIGN_BYTE			0x00f8

EAType g_eEAType = EAT_NULL;

/// initial permutation (��ʼ�û�����)
unsigned char ip[] = {
	58, 50, 42, 34, 26, 18, 10,  2,
	60, 52, 44, 36, 28, 20, 12,  4,
	62, 54, 46, 38, 30, 22, 14,  6,
	64, 56, 48, 40, 32, 24, 16,  8,
	57, 49, 41, 33, 25, 17,  9,  1,
	59, 51, 43, 35, 27, 19, 11,  3,
	61, 53, 45, 37, 29, 21, 13,  5,
	63, 55, 47, 39, 31, 23, 15,  7
};


/// final permutation (����ת������)
unsigned char fp[] = {
	40,  8, 48, 16, 56, 24, 64, 32,
	39,  7, 47, 15, 55, 23, 63, 31,
	38,  6, 46, 14, 54, 22, 62, 30,
	37,  5, 45, 13, 53, 21, 61, 29,
	36,  4, 44, 12, 52, 20, 60, 28,
	35,  3, 43, 11, 51, 19, 59, 27,
	34,  2, 42, 10, 50, 18, 58, 26,
	33,  1, 41,  9, 49, 17, 57, 25
};

/// permuted choice 1 (KEY�û�ѡ���PC-1)
unsigned char pc1[] = {
	57, 49, 41, 33, 25, 17,  9,
	1,  58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27,
	19, 11,  3, 60, 52, 44, 36,

	63, 55, 47, 39, 31, 23, 15,
	7,  62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29,
	21, 13,  5, 28, 20, 12,  4
};

/// permuted choice 2 (KEY�û�ѡ���PC-2)
unsigned char pc2[] = {
	14, 17, 11, 24,  1,  5,
	3,  28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8,
	16,  7, 27, 20, 13,  2,
	41, 52, 31, 37, 47, 55,
	30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53,
	46, 42, 50, 36, 29, 32
};

/// Ci��Di�ĵ����ƺ��� (���PC-1)
unsigned char ls[] = { 1, 2, 4, 6, 8, 10, 12, 14, 15, 17, 19, 21, 23, 25, 27, 28 };

/// 48 --> 32λѡ���
unsigned char si[8][64]
= {
	/// S[1]
	14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
	0,  15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
	4,   1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,
	/// S[2]
	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	3,  13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	0,  14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,
	/// S[3]
	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	1,  10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,
	/// S[4]
	7,  13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	3,  15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,
	/// S[5]
	2,  12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	4,   2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,
	/// S[6]
	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	9,  14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	4,   3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,
	/// S[7]
	4,  11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	1,   4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	6,  11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,
	/// S[8]
	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	1,  15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	7,  11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	2,   1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

/// 32-bit permutation function (��ѡ�������û�����)
unsigned char p[] = {
	16,  7, 20, 21,
	29, 12, 28, 17,
	1,  15, 23, 26,
	5,  18, 31, 10,
	2,   8, 24, 14,
	32, 27,  3,  9,
	19, 13, 30,  6,
	22, 11,  4, 25
};

unsigned char iperm[8][256][8], fperm[8][256][8];	/// ��ʼ�������û�����
int bytebit[] = {									/// ������Ϊ B1B2B3B4B5B6B7B8
	0x80, 0x40, 0x20, 0x10,
	0x08, 0x04, 0x02, 0x01
};
int keybit[28] = {									/// 28λ��Կ�벿
	0x80000000, 0x40000000, 0x20000000, 0x10000000, 0x08000000, 0x04000000, 0x02000000, 0x01000000,
	0x00800000, 0x00400000, 0x00200000, 0x00100000, 0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000, 0x00000800, 0x00000400, 0x00000200, 0x00000100,
	0x00000080, 0x00000040, 0x00000020, 0x00000010
};

unsigned char s[4][4096];							/// s1-s8ѡ����û�����
unsigned char p32[4][256][4];						/// 32λѡ�������û�����
unsigned char pc1m[56];								/// �û������Կ
unsigned char pcr[56];								/// ���ƺ����Կ

void DESPermInit(unsigned char perm[8][256][8],
				 unsigned char p[64]);				/// �����û����г�ʼ�û�����
void SelectFuncInit(void);							/// ѡ������ʼ��(s1-s8)
unsigned char Compress6(int k, int v);				/// ����skѹ��6-bitΪ4-bit
void P32Init(void);									/// 32λ�û�������ʼ��
void EnDES(unsigned char *inblock,
		   const unsigned char kn[16][6]);			/// ��64λ���ݿ����DES����
void DeDES(unsigned char *inblock,
		   const unsigned char kn[16][6]);			/// ��64λ���ݿ����DES����
void Permute(unsigned char *inblock,				/// ʹ���û�������������ݽ����û�
			 unsigned char perm[8][256][8],
			 unsigned char *outblock);
void Iter(int i, unsigned char *inblock,
		  unsigned char *outblock,
		  const unsigned char kn[16][6]);			/// ��i�ּ���
void DelayOutput(unsigned char *block,
				 unsigned char flag);				/// 1-block delay on output 
void CipherFunc(unsigned char * right, int i,
				unsigned char *fret,
				const unsigned char kn[16][6]);		/// ���L[i-1]�ļ��ܺ���f
void Perm32(unsigned char *inblock,
			unsigned char *outblock);				/// ��32λѡ��������P�û�
void Expand(unsigned char *right,
			unsigned char *bigright);				/// 32-bit --E-> 48-bit
void Contract(unsigned char *in48,
			  unsigned char *out32);				/// 48-bit --S-> 32-bit
int Garbage(void);									/// �����������

/// ��ʼ��
void EAInit(EAType eT)
{
	g_eEAType = eT;
#ifdef _DEBUG
	printf("�Գ�ʼ�û��������г�ʼ��.\n");
#endif
	DESPermInit(iperm, ip);							/// ��ʼ�û�����IP
#ifdef _DEBUG
	printf("�������û��������г�ʼ��.\n");
#endif
	DESPermInit(fperm, fp);							/// �����û�����FP
#ifdef _DEBUG
	printf("����ѹ����ʼ��(s1-s8).\n");
#endif
	SelectFuncInit();								/// ѡ������ʼ��(s1-s8)

#ifdef _DEBUG
	printf("32λ�û���ʼ��.\n");
#endif
	P32Init();										/// 32λ�û�����P
#ifdef _DEBUG
	printf("��ʼ�����.\n");
#endif
}

/// ������Կ
void KeyInit(const unsigned char *pKey, unsigned char kn[16][6])
{
#ifdef _DEBUG
	printf("��ʼ����Կ��.\n");
#endif

	register int i, j, l;
	int m;

	for (j=0; j<56; j++)							/// ��Կ��PC-1�û�
	{
		l = pc1[j] - 1;								/// ��Կ�е�λ��(bit)
		m = l & 0x07;								/// l���ֽ��е�λ��(bit)
		pc1m[j] = (pKey[l>>3] &						/// ��Կl>>3�ֽ�mλ��ֵ(0 or 1)����pc1m
			bytebit[m])
			? 1 : 0;
	}

	for (i=0; i<16; i++)							/// ��Կ����0
		for (j=0; j<6; j++)
			kn[i][j] = 0;

	for (i=0; i<16; i++)							/// �����1�ֵ�KEY
	{
		for (j=0; j<56; j++)						/// ѭ������
			pcr[j] = pc1m[(l=j+ls[i])<(j<28? 28 : 56) ? l: l-28];

		for (j=0; j<48; j++)
			if (pcr[pc2[j]-1])
			{
				l = j & 0x07;
				kn[i][j>>3] |= bytebit[l];
			}
	}
}

/// �����ⳤ�����ݽ��м���
int Encrypt(unsigned char *pBlock, int l, const unsigned char kn[16][6])
{
	register int j, k;
	register unsigned char *b;

	k = l > ENCRYPT_SIZE ? ENCRYPT_SIZE : (l & ALIGN_BYTE);
	for (b = pBlock, j = 0; k > 0; k -= 8, b += 8)
	{
		EnDES(b, kn);
	}

	return k;
}

/// �����ⳤ�����ݽ��н���
int Decrypt(unsigned char *pBlock, int l, const unsigned char kn[16][6])
{
	register int k;
	register unsigned char *b;

	k = l > ENCRYPT_SIZE ? ENCRYPT_SIZE : (l & ALIGN_BYTE);
	for (b = pBlock; k > 0; k -= 8, b += 8)
	{
		DeDES(b, kn);
	}

	return k;
}

/// ��64λ���ݿ����DES����
void EnDES(unsigned char *inblock, const unsigned char kn[16][6])
{
	unsigned char iters[17][8];						/// ����16�ֱ任������
	unsigned char swap[8];							/// ���ڽ���L & R
	register int i;
	register unsigned char *s, *t;

	Permute(inblock, iperm, iters[0]);				/// IP�û�
	for (i=0; i<16; i++)							/// 16�ּ���
		Iter(i, iters[i], iters[i+1], kn);
	s = swap;										/// R16<<4B
	t = &iters[16][4];
	*s++ = *t++; *s++ = *t++; *s++ = *t++; *s++ = *t++;
	t = &iters[16][0];								/// L16>>4B
	*s++ = *t++; *s++ = *t++; *s++ = *t++; *s++ = *t++;
	Permute(swap, fperm, inblock);					/// FP�û�
}

/// ��64λ���ݿ���н���
void DeDES(unsigned char *inblock, const unsigned char kn[16][6])
{
	unsigned char iters[17][8];						/// ����16�ֱ任������
	unsigned char swap[8];							/// ���ڽ���L & R
	register int i;
	register unsigned char *s, *t;

	Permute(inblock, iperm, iters[0]);				/// IP�û�
	for (i=0; i<16; i++)							/// 16�ֽ���
		Iter(15-i, iters[i], iters[i+1], kn);
	s = swap;
	t = &iters[16][4];								/// R0<<4B
	*s++ = *t++; *s++ = *t++; *s++ = *t++; *s++ = *t++;
	t = &iters[16][0];								/// L0>>4B
	*s++ = *t++; *s++ = *t++; *s++ = *t++; *s++ = *t++;
	Permute(swap, fperm, inblock);					/// FP�û�
}

/// �����������
int Garbage(void)
{
	return rand() & 0xff;
}

/// ʹ���û�������������ݽ����û�
void Permute(unsigned char *inblock, unsigned char perm[8][256][8], unsigned char *outblock)
{
	register int i, j;
	register unsigned char *ib, *ob;
	register unsigned char *p;

	for (i=0, ob = outblock; i<8; i++)
		*ob++ = 0;

	ib = inblock;
	for (j = 0; j < 8; j++, ++ib)
	{
		ob = outblock;
		p = perm[j][*ib];							/// 8λ����
		for (i = 0; i < 8; i++)
			*ob++ |= *p++;							/// �ϲ�����
	}
}

unsigned char Compress6(int k, int v)				/// ����skѹ��6-bitΪ4-bit
{
	register int i, j;

	i = ((v&0x20) >> 4) | (v&1);					/// D1& D5 -> i��
	j = (v&0x1f) >> 1;								/// D2D3D4D5 -> j��
	return si[k][(i<<4) + j];						/// si��i��j��ֵ
}

/// ѡ������ʼ��(s1-s8)
void SelectFuncInit(void)
{
	register int i, j;

	for (i=0; i<4; i++)								/// 12-bit->8-bit
		for (j=0; j<4096; j++)						/// 12-bit�������п��ܵ�ö��
			s[i][j] = (Compress6(i*2, j>>6) << 4) |	/// ��4λ
			(0x0f & Compress6(i*2+1, j & 0x3f));	/// ��4λ
}

/// 32λ�û�������ʼ��
void P32Init(void)
{
	register int l, j, k;
	int i,m;

	for (i=0; i<4; i++)								/// �����ֽ�
		for (j=0; j<256; j++)						/// ÿ1�ֽڿ���ȡֵ��ö��
			for (k=0; k<4; k++)
				p32[i][j][k]=0;
	for (i=0; i<4; i++)								/// �����ֽ�
		for (j=0; j<256; j++)						/// ÿ1�ֽڿ���ȡֵ��ö��
			for (k=0; k<32; k++)					/// ÿһλ�û����λ��
			{
				l = p[k] - 1;
				if ((l>>3) != i)					/// l�Ƿ�λ�ڵ�i�ֽ�
					continue;
				if (!(j & bytebit[l&0x07]))
					continue;						/// j�Ƿ���l������ͬλֵΪ1
				m = k & 0x07;						/// ��1λ��λ��
				p32[i][j][k>>3] |= bytebit[m];
			}
}

/// �����û����г�ʼ�û�����
void DESPermInit(unsigned char perm[8][256][8], unsigned char p[64])
{
	register int l, j, k;
	int i, m;

	for (i=0; i<8; i++)
		for (j=0; j<256; j++)
			for (k=0; k<8; k++)
				perm[i][j][k] = 0;					/// ����û�����

	for (i=0; i<8; i++)								/// �����ֽ�
		for (j = 0; j<256; j++)						/// ÿ1�ֽڿ���ȡֵ��ö��
			for (k = 0; k<64; k++)					/// ���λ��λ��
			{
				l = p[k] - 1;						/// k��Ӧ����λ
				if ((l >> 3) != i)					/// l���ֽ���� == i
					continue;
				if (!(j & bytebit[l & 0x07]))		/// j�ĵ�l&0x07λΪ1
					continue;
				m = k & 0x07;						/// 1B�е�λ��
				perm[i][j][k>>3] |= bytebit[m];		/// l�����ֽڵ�λ��
			}
}

/// ��i�ּ���
void Iter(int i, unsigned char *inblock, unsigned char *outblock, const unsigned char kn[16][6])
{
	unsigned char fret[4];							/// ���f(R,K)�ļ��ܽ��
	register unsigned char *ib, *ob, *fb;

	ob = outblock;
	ib = &inblock[4];								/// R[i-1]
	CipherFunc(ib, i, fret, kn);						/// f(R, K)
	*ob++ = *ib++;									/// L[i] = R[i-1]
	*ob++ = *ib++;
	*ob++ = *ib++;
	*ob++ = *ib++;
	ib = inblock;
	fb = fret;										/// R[i] = L[i] XOR f(L[i],K) 
	*ob++ = *ib++ ^ *fb++;
	*ob++ = *ib++ ^ *fb++;
	*ob++ = *ib++ ^ *fb++;
	*ob++ = *ib++ ^ *fb++;
}

/// ��L[i-1]���м���
void CipherFunc(unsigned char * right, int i, unsigned char *fret, const unsigned char kn[16][6])
{
	register const unsigned char *kb;
	register unsigned char *rb, *bb;
	unsigned char rightex[6];						/// ��չ�Ҳ�
	unsigned char result[6];						/// ���Expand(R) XOR KEY[i]
	unsigned char preout[4];						/// ���32-bit P�û����

	kb = kn[i];										/// ��i����Կ
	bb = rightex;
	rb = result;
	Expand(right, bb);								/// 32-bit --E-> 48-bit
	*rb++ = *bb++ ^ *kb++;							/// Expand(R) XOR KEY[i] (48-bit)
	*rb++ = *bb++ ^ *kb++;
	*rb++ = *bb++ ^ *kb++;
	*rb++ = *bb++ ^ *kb++;
	*rb++ = *bb++ ^ *kb++;
	*rb++ = *bb++ ^ *kb++;
	Contract(result, preout);						/// 48-bit --S-> 32-bit
	Perm32(preout, fret);							/// ��32λѡ��������P�û�
}

/// ��32λѡ��������P�û�
void Perm32(unsigned char *inblock, unsigned char *outblock)
{
	register int i;
	register unsigned char *ib, *ob;
	register unsigned char *q;

	ob = outblock;
	*ob++ = 0;
	*ob++ = 0;
	*ob++ = 0;
	*ob++ = 0;
	ib = inblock;
	for (i=0; i<4; i++, ib++)
	{
		q = p32[i][*ib & 0xff];
		ob = outblock;
		*ob++ |= *q++;
		*ob++ |= *q++;
		*ob++ |= *q++;
		*ob++ |= *q++;
	}
}

/// 32-bit --E-> 48-bit
void Expand(unsigned char *right, unsigned char *bigright)
{
	register unsigned char *bb, *r, r0, r1, r2, r3;

	bb = bigright;
	r = right;
	r0 = *r++;
	r1 = *r++;
	r2 = *r++;
	r3 = *r++;

	*bb++ = ((r3 & 0x01) << 7) |					/// 32
		((r0 & 0xf8) >> 1) |						/// 1 2 3 4 5
		((r0 & 0x18) >> 3);							/// 4 5
	*bb++ = ((r0 & 0x07) << 5) |					/// 6 7 8
		((r1 & 0x80) >> 3) |						/// 9
		((r0 & 0x01) << 3) |						/// 8
		((r1 & 0xe0) >> 5);							/// 9 10 11
	*bb++ = ((r1 & 0x18) << 3) |					/// 12 13
		((r1 & 0x1f) << 1) |						/// 12 13 14 15 16
		((r2 & 0x80) >> 7);							/// 17
	*bb++ = ((r1 & 0x01) << 7) |					/// 16
		((r2 & 0xf8) >> 1) |						/// 17 18 19 20 21
		((r2 & 0x18) >> 3);							/// 20 21
	*bb++ = ((r2 & 0x07) << 5) |					/// 22 23 24
		((r3 & 0x80) >> 3) |						/// 25
		((r2 & 0x01) << 3) |						/// 24
		((r3 & 0xe0) >> 5);							/// 25 26 27
	*bb++ = ((r3 & 0x18) << 3) |					/// 28 29
		((r3 & 0x1f) << 1) |						/// 28 29 30 31 32
		((r0 & 0x80) >> 7);							/// 1
}

/// 48-bit --S-> 32-bit
void Contract(unsigned char *in48, unsigned char *out32)
{
	register unsigned char *c;
	register unsigned char *i;
	register int i0, i1, i2, i3, i4, i5;

	i = in48;
	i0 = *i++;
	i1 = *i++;
	i2 = *i++;
	i3 = *i++;
	i4 = *i++;
	i5 = *i++;
	c = out32;
	*c++ = s[0][0x0fff & ((i0 << 4) | ((i1 >> 4) & 0x0f  ))];
	*c++ = s[1][0x0fff & ((i1 << 8) | ( i2 & 0xff ))];
	*c++ = s[2][0x0fff & ((i3 << 4) | ((i4 >> 4) & 0x0f  ))];
	*c++ = s[3][0x0fff & ((i4 << 8) | ( i5 & 0xff ))];
}