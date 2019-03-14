#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <random>

#define debug 0

int frame = 0;
int length = 1024;
const int error_target = 200;
int times;
double stddev;
double Eb_N0_dB;

using namespace::std;

double rand_normal(double mean, double stddev)
{//Box muller method


    static double n2 = 0.0;
    static int n2_cached = 0;
    if (!n2_cached)
    {
        double x, y, r;
        do
        {
            x = 2.0*rand()/RAND_MAX - 1;
            y = 2.0*rand()/RAND_MAX - 1;

            r = x*x + y*y;
        }
        while (r == 0.0 || r > 1.0);
        {
            double d = sqrt(-2.0*log(r)/r);
            double n1 = x*d;
            n2 = y*d;
            double result = n1*stddev + mean;
            n2_cached = 1;
            return result;
        }
    }
    else
    {
        n2_cached = 0;
        return n2*stddev + mean;
    }
}

struct sol
{
	sol* pre;
	int state;
};

int main()
{
    if (!debug) srand(time(NULL));
    //std::default_random_engine generator;
    
    FILE *f = fopen("Viterbi.txt","w");
	fclose(f);

	for (Eb_N0_dB=-3;Eb_N0_dB<7.41;Eb_N0_dB+=0.1)
	{
		//std::normal_distribution<double> distribution(0,1./SNR);
		stddev = sqrt(pow(10,-Eb_N0_dB/10));
		times = 5;
		printf ("SNR = %lf, stddev = %lf",Eb_N0_dB,stddev);
		if (debug) printf ("\n");
	    while (times)
	    {
	    	int error_count = 0;
	    	frame = 0;
	    	while(1)
	    	{
	    		frame++;
	    		int* message_array;
		        message_array = new int[length];
		        double* coded_array;
		        coded_array = new double[length*2];
		        double* noise_array;
		        noise_array = new double[length*2];
		        double* receive_array;
		        receive_array = new double[length*2];
		
		        int reg0=0;
		        int reg1=0;
		        int reg2=0;
		
		        for (int i=0;i<length;i++)      //construct initial message
		        {
		            if (i>length-4) message_array[i]=0;
		            else message_array[i] = rand()/0.5 > RAND_MAX ? 1 : 0;
		            if (debug) printf ("%d ",message_array[i]);
		            reg2 = reg1;
		            reg1 = reg0;
		            reg0 = message_array[i];
		            coded_array[2*i  ] = (reg0+reg1+reg2)%2 ? 1 : -1;
		            coded_array[2*i+1] = (reg0+reg2)%2 ? 1 : -1;
		        }
		        if (debug) printf ("\n");
		        for (int i=0;i<2*length;i++)
		        {
		            noise_array[i] = rand_normal(0,stddev);
		            receive_array[i] = coded_array[i] + noise_array[i];
		            if (debug) printf ("%+.1lf ",receive_array[i]);
		        }
		        if (debug) printf ("\n");
		        delete [] coded_array;
		        delete [] noise_array;
		
				//Viterbi
				double error[4] = {0,0,0,0};
				sol** state = new sol* [4];
				for (int i=0;i<4;i++)
				{
					state[i] = new sol[length];
				}
				
		        int route[4][4] = {{0,1,-1,-1},{-1,-1,2,3},{4,5,-1,-1},{-1,-1,6,7}};
		        int a[8] = {0,1,1,0,1,0,0,1};
		        int b[8] = {0,1,0,1,1,0,1,0};
		        
				for (int i=0;i<length;i++)
				{
					sol* old_state[4];
					double new_error[4];
					for (int j=0;j<4;j++)
					{
						state[j][i].state = j;
						if (i==0)
						{
							error[j] = (j==0 || j==1)?receive_array[2*i]*a[route[0][j]]+receive_array[2*i+1]*b[route[0][j]]:-1000;
						}	
						else
						{
							int source = -1;
							double max_error = -1;
							for (int k=0;k<4;k++)
							{
								if (route[k][j]>=0)
								{
									if (max_error<error[k]+receive_array[2*i]*a[route[k][j]]+receive_array[2*i+1]*b[route[k][j]])
									{
										max_error=error[k]+receive_array[2*i]*a[route[k][j]]+receive_array[2*i+1]*b[route[k][j]];
										source = k;
									}
								}
							}
							new_error[j] = max_error;
							old_state[j] = &state[source][i-1];
						}
					}
					for (int j=0;j<4;j++)
					{
						state[j][i].pre = old_state[j];
						error[j] = new_error[j];
					}
				}
				
				int* decoded_array;
		        decoded_array = new int[length];
		        sol* ptr = &state[0][length-1];
		        for (int i=length-1;i>=0;i--)
		        {
		        	decoded_array[i] = ptr->state%2;
		        	ptr = ptr->pre;
				}
		        
				//Summary
		        for (int i=0;i<length;i++)
		            if (decoded_array[i]!=message_array[i])
					{
						error_count++;
						break;					//Frame error rate
					} 
		
		        delete [] message_array;
		        delete [] receive_array;
		        for (int i=0;i<4;i++)
		        {
		        	delete [] state[i];
				}
				delete [] state;
		        delete [] decoded_array;
		        //printf ("delete\n");
		        //scanf ("%c",&temp);
		        if (error_count>=error_target) break;
			}
			printf (", error count = %d\n",error_count);
			FILE *f = fopen("Viterbi.txt","a");
		    //fprintf (f,"%.1lf %d %d\n",Eb_N0_dB,error_count,length*frame);		//bit error rate
		    fprintf (f,"%.1lf %d %d\n",Eb_N0_dB,error_count,frame);					//frame error rate
		    fclose(f);
			
		    char temp;
		    if (debug) scanf ("%c",&temp);
		    
		    
		    times--;
	    }
	}

    return 0;
}

