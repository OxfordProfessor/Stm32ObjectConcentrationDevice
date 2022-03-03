#include "contral.h"

//0��PID��ʼ������������������ֵ
//����(4��)��Kp��Ki��Kd�������PID�ṹ��ĵ�ַ
void pid_init(float Kp, float Ki, float Kd, PID_TypeDef* PID)
{
	PID->Kp = Kp;
	PID->Ki = Ki;
	PID->Kd = Kd;
}

//1��λ��PID
//����(3��)����ǰλ�ã�Ŀ��λ�ã������PID�ṹ��ĵ�ַ
int pid(int present, u16 target, PID_TypeDef* PID)
{
	PID->error = target-present;	//������� = Ŀ��ֵ - ʵ��ֵ
	
	PID->p_out = PID->Kp * PID->error;//����
	PID->i_out += PID->Ki * PID->error;//����
	PID->d_out = PID->Kd * (PID->error - PID->last_error);//΢��
	
	PID->output = PID->p_out + PID->i_out + PID->d_out;//���
	
	PID->last_error = PID->error;//�ϴ���� = �������
	
	return PID->output;
}


//2���Ľ���λ��PID(��΢������и��ƣ�������ʷ��Ϣ������)
//����(3��)����ǰλ�ã�Ŀ��λ�ã������PID�ṹ��ĵ�ַ
int better_pid(int present, u16 target, PID_TypeDef* PID)
{
	PID->error = target-present;	//������� = Ŀ��ֵ - ʵ��ֵ
	
	PID->p_out = PID->Kp * PID->error;//����
	PID->i_out += PID->Ki * PID->error;//����
	PID->d_out = PID->Kd * 1/16 * (PID->error + 3*PID->last_error + 2*PID->last_error2 -2*PID->last_error3 - 3*PID->last_error4 - PID->last_error5);//΢��
	
	PID->output = PID->p_out + PID->i_out + PID->d_out;//���
	
	PID->last_error5 = PID->last_error4;//�ϴ���� = �������
	PID->last_error4 = PID->last_error3;//�ϴ���� = �������
	PID->last_error3 = PID->last_error2;//�ϴ���� = �������
	PID->last_error2 = PID->last_error;//�ϴ���� = �������
	PID->last_error = PID->error;//�ϴ���� = �������
	
	return PID->output;
}


//3������ʽPID(ĿǰЧ������)
//����(3��)����ǰλ�ã�Ŀ��λ�ã������PID�ṹ��ĵ�ַ
int incre_pid(int present, u16 target, PID_TypeDef* PID)
{
	PID->error = target-present;	//������� = Ŀ��ֵ - ʵ��ֵ
	
	PID->p_out = PID->Kp * (PID->error - PID->last_error);//����
	PID->i_out += PID->Ki * PID->error;//����
	PID->d_out = PID->Kd * (PID->error - 2*PID->last_error + PID->last_error2);//΢��
	
	PID->output += PID->p_out + PID->i_out + PID->d_out;//���
	
	PID->last_error2 = PID->last_error;
	PID->last_error = PID->error;//�ϴ���� = �������
	
	return PID->output;
}
