#pragma once

/// ���ݼ����㷨
enum EAType
{
	EAT_NULL	= 0x00,
	EAT_DES		= 0x01,
	EAT_TDEA	= 0x02,
};

void EAInit(EAType eT = EAT_DES);					/// ��ʼ��
void KeyInit(const unsigned char *pKey,
			unsigned char kn[16][6]);				/// ������Կ
int Encrypt(unsigned char *pBlock,
			int l,
			const unsigned char kn[16][6]);			/// �����ⳤ�����ݽ��м���
int Decrypt(unsigned char *pBlock,
			int l,
			const unsigned char kn[16][6]);			/// �����ⳤ�����ݽ��н���