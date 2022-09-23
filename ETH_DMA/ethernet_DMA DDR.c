#include "../driver/encoding.h"
#include "../driver/uart.h"
#include "stdio.h"
#include "type.h"
#include "stdlib.h"
#include "uart.h"
#include "string.h"
#include "spi.h"
#include "encoding.h"

#define test_filter 1

#define test_drop 0
#define interrupt_enable 0
#define check_eth 1
#define set_eth 1
#define ping_DHCP 1
#define check_ping 1

    int i,size,count;
    int LINK_flag = 1;

int int2str(char * str, int num){
    int negative = 0;
    char buf[100] = {0};
    int len = 0, k = 0, i = 0, tmp = 0, n = 0, lenstr = 0;
    if(num < 0){
        negative = 1;
        n = -num;
    } else {
        n = num;
    }
    do{
        tmp = n % 10;
        buf[k++] = tmp + '0';
        n = n / 10;
    } while(n != 0);
    len = k; /*len does not need to be +1, because k++ is already */
    lenstr = len;
    k = 0;
    if(1 == negative){
        str[k++] = '-';
        lenstr = lenstr + 1;
    }
    for(i = len - 1; i >= 0; i--){
        str[k++] = buf[i];
    }
    str[lenstr] = '\0';
    return *str;

}


void vSendString( const char * pcString )
{
    volatile char *read_temp = (char *) 0x60000004;
        volatile char *stat_temp = (char *) 0x60000008;

        while( *pcString != 0x00 )
        {
            while((*stat_temp & 0x8)==0x8);
            *read_temp = *pcString;
            pcString++;
        }
}

#define Timer0_crl   0x60004000
#define Timer0_lr   0x60004004
#define Timer0_cr   0x60004008

#define U32         *(volatile unsigned int *)
#define PLIC_BASE   0x0C000000
#define UART0_BASE   0x60000000

void handle_trap();
void csr_cfg();
void PLIC_init_cfg(unsigned int num, unsigned int threshold);
void PLIC_id_clr(unsigned int id);
void PLIC_sourceX_cfg(unsigned int num, unsigned int priority, unsigned int enable);
unsigned int PLIC_id_read();
unsigned int PLIC_read_pending();
void serial_init();



//CPC process
void CPC(){
    volatile u32 rs1 ;
    volatile u32 rs2 ;
                                    rs1=0x80001000;
                                    rs2=0x80001000;
                                    //CPC
                                    *((volatile u32*) 0x60030000)= 1;                             //Purge Cache
                                    //Dummy ROCC
                                    volatile u32 * rd1 = (volatile u32 *)0x60030000;                  //CPC reg
                                    asm volatile ( ".insn r CUSTOM_0, 7, 6, %0, %1, %2"
                                                                                                                    :               "=r"(*rd1)
                                                                                                                    :               "r"(rs1), "r"(rs2) );
}





//delay function
void delay(u32 x){
    volatile u32 *time_crl = (u32 *) Timer0_crl;
    volatile u32 *time_lr = (u32 *) Timer0_lr;
    volatile u32 *time_cr = (u32 *) Timer0_cr;

    //volatile char *read = (char *) base_BASE;
    //char boolean;
    //volatile unsigned int *writest = (unsigned int *) base_BASE + Tx_BASE;

    *time_lr = 0x00000000;                   //reset load


    *time_cr = 0x00000000;
    *time_crl = 0x00000000;     ////clean timer control

    *time_lr = 0x00000001;

    //check = *time_lr;
    //if(check == 0x00000001){
    //  uart_writeStr(base_BASE,"\nload is correct\n\r");
    //}else{uart_writeStr(base_BASE,"\nload not correct\n\r");}


    *time_crl = *time_crl | 0x00000034;          // turn on load register and GENT0

    //check = *time_cr;
    //if(check == 0x00000001){
    //  uart_writeStr(base_BASE,"\ncounter is correct\n\r");
    //}else{uart_writeStr(base_BASE,"\ncounter not correct\n\r");}

    *time_crl = *time_crl & 0x00000014;          // turn off load register
    *time_crl = *time_crl | 0x00000080;          // turn on timer

    x = x*0x02FFFFFF;

while(*time_cr < x){}

}





//--------------------------------------------------------------------------
// handle_trap function

void handle_trap(unsigned int cause, unsigned int epc, unsigned int regs[32])
{

//  delay(1);
    unsigned int id_temp;
//    uart_writeStr(UART0_BASE, "\n\r Entered trap");
    //enter handle_trap()
    //print PLIC interrupt id
    id_temp = PLIC_id_read();
    //clear the PLIC interrupt id
    if(id_temp == 13){
        vSendString("\n\rRI\n\r");  //recv interrupt
        //RECV interrupt handle
    }else if(id_temp == 14){
        vSendString("\n\rTI\n\r");  //trans empty interrupt
        //TRAN interrupt handle
    }
    PLIC_id_clr(id_temp);
    //if PLIC pending is not 0x0, some other interrupts is pending
    while(PLIC_read_pending() != 0x0)
    {
        //print PLIC interrupt id
        id_temp = PLIC_id_read();
        //clear the PLIC interrupt id
        PLIC_id_clr(id_temp);
    }
}

//--------------------------------------------------------------------------
// CSR interrupt configuration function

void csr_cfg()
{

//  volatile u32 mie = (u32 *) 0x304;
//  volatile u32 mstatus = (u32 *) 0x300;

    unsigned int csr_tmp;

    //mie.MEIE
    write_csr(mie,0x0);
   csr_tmp = read_csr(mie);
//    write_csr(mie,(csr_tmp | 0xFFF  F0888));
    write_csr(mie,(csr_tmp | 0x800));

    //mstatus.MIE
    csr_tmp = read_csr(mstatus);
    //write_csr(mstatus,0x0);
    write_csr(mstatus,(csr_tmp | 0x8));


//   mie = 0;
//   mie = mie | 0x800;
//   mstatus = 0x8 | mstatus;// turn on machine interrupt enable

}

//--------------------------------------------------------------------------
// PLIC initial configuration function

void PLIC_init_cfg(unsigned int num, unsigned int threshold)

{
    char boolean;
    volatile u32 *test = (u32 *) 0x0C000008;
    unsigned int i;


    //source x priority
    for(i=1; i<=num; i++)
    {
        U32(PLIC_BASE + 0x0000 + 4*i) = 0x0;
    }

    //PLIC interrupt enable0
    U32(PLIC_BASE + 0x2000) = 0x0;
    //PLIC interrupt enable1
    if(num >= 32)
    {
        U32(PLIC_BASE + 0x2004) = 0x0;
    }

    //set PLIC interrupt threshold
    U32(PLIC_BASE + 0x200000) = threshold; //M-mode threshold

//    if(*test == 0){
//      uart_writeStr(UART0_BASE, "\n\rPLIC init cfg setting correct");
//    }else{uart_writeStr(UART0_BASE, "\n\rPLIC init cfg setting incorrect");}
//
//  uart_writeStr(UART0_BASE, "\n\rPLIC init cfg END");
}

//--------------------------------------------------------------------------
// PLIC ID clear function

unsigned int PLIC_id_read()
{

    //read PIIL interrupt id
    unsigned int id;

    id = U32(PLIC_BASE + 0x200004);
    return id;
}

void PLIC_id_clr(unsigned int id)
{

    //clear PIIL interrupt id
    U32(PLIC_BASE + 0x200004) = id;
}

//--------------------------------------------------------------------------
// PLIC read pending function

unsigned int PLIC_read_pending()
{
    //read PIIL interrupt pending
    unsigned int pending;

    pending = U32(PLIC_BASE + 0x1000);
    return pending;
}

//--------------------------------------------------------------------------
// PLIC sourceX configuration function

void PLIC_sourceX_cfg(unsigned int num, unsigned int priority, unsigned int enable)
{

    volatile u32 *test = (u32 *) 0x0C000004;
    //set source X priority
    U32(PLIC_BASE + 0x0000 + 4*num) = priority;
    //set source X enable
    U32(PLIC_BASE + 0x2000) = U32(PLIC_BASE + 0x2000) | (enable<<num);

//    if(*test == 2){
//      uart_writeStr(UART0_BASE, "\n\rPLIC sourceX cfg setting correct");
//    }else{uart_writeStr(UART0_BASE, "\n\rPLIC sourceX cfg setting incorrect");}
//
//  uart_writeStr(UART0_BASE, "\n\rPLIC sourceX cfg END");

}

void serial_init()
{
    U32(UART0_BASE + 0xc) = 0x13;        // enable interrupt and reset RX TX

//  uart_writeStr(UART0_BASE, "\n\rserial init END");
}

//--------------------------------------------------------------------------
// Main




int main (void){

    unsigned int csr_tmp;
    //enable ROCC
    #define read_csr(reg) ({ unsigned long __tmp; \
    asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp; })

    #define write_csr(reg, val) ({ \
    asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

    //enable accelerator if present

        csr_tmp = read_csr(mstatus);
        csr_tmp = csr_tmp | 0x00018008;
    //            csr_tmp = csr_tmp | 0x00018000;
        write_csr(mstatus, csr_tmp);




#if interrupt_enable

    //interrupt config
//  PLIC_init_cfg(0x1,0x1);
    PLIC_init_cfg(0xD,0x1);
    PLIC_init_cfg(0xE,0x1);

//    PLIC_sourceX_cfg(0x1,0x2,0x1);
    PLIC_sourceX_cfg(0xD,0x2,0x1);  /// Int12                   Ethernet data received packet is ready
    PLIC_sourceX_cfg(0xE,0x2,0x1);  /// Int13                   Ethernet transmitted buffer is empty
    csr_cfg();
    serial_init();

#endif

    unsigned int recv_data;
    unsigned int ethernet_wr_ready_addr = 0x60008008;
    unsigned int ethernet_tran_addr = 0x60008004;
    unsigned int drop_addr = 0x60009010;
//  unsigned int ethernet_recv_addr = 0x81000000;

    unsigned int interrupt_enb = 0x60009024;

    volatile unsigned int *interrupt_enb_data = (unsigned int *) interrupt_enb;

    *interrupt_enb_data = 0x0; //disable all interrupt

    volatile unsigned int *ethernet_wr_ready = (unsigned int *) ethernet_wr_ready_addr;
    volatile unsigned int *tran_data = (unsigned int *) ethernet_tran_addr;
    volatile unsigned int *drop_count = (unsigned int *) drop_addr;
//  volatile unsigned int *recv_data = (unsigned int *) ethernet_recv_addr;



    unsigned int temp_tran;

#if test_drop

    unsigned int drop_count_addr = 0x6000900C;
    volatile unsigned int *drop_count = (unsigned int *) drop_count_addr;

    *drop_count = 4046;

    if(*drop_count == 4046){
        vSendString("Drop count correct\n\r");
    }else{
        vSendString("Drop count INcorrect\n\r");
    }



#endif

#if set_eth

    vSendString("\n\rStart testing ethernet\n\r");


    *ethernet_wr_ready = 0;// first time reset

    //ethernet tran data flow
    *tran_data = 0x4201000;     //enable PCS loopback mode   speed 100MHz   Enable Auto-Negotiation mode
    temp_tran = 0x1000;

    while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy

    //ethernet read data flow
    vSendString("Check auto Negotiation\n\r");
    recv_data = 0;

    while(recv_data & 0x20 != 0x20){
    *tran_data = 0x8210000; ///TEST

    while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
    while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
    *ethernet_wr_ready = 0;     //reset ready flag

    recv_data = *tran_data;
    }

    vSendString("Auto Negotiation COMPLETE\n\r");

#endif

#if check_eth
    recv_data = 0;

    *tran_data = 0x8310000; ///TEST

    while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
    while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
    *ethernet_wr_ready = 0;     //reset ready flag

    recv_data = *tran_data;

    if((recv_data & 0x400) != 0x400){
        vSendString("Ethernet NOT LINKED\n\r");

    while((recv_data & 0x400) != 0x400){

    *tran_data = 0x8310000; ///TEST

    while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
    while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
    recv_data = *tran_data;
    *ethernet_wr_ready = 0;     //reset ready flag
    LINK_flag = 0;

//  recv_data = *tran_data;
    }
    }else{}

    if(LINK_flag == 0){
        //ethernet tran data flow
        *tran_data = 0x4201000;     //enable PCS loopback mode   speed 100MHz   Enable Auto-Negotiation mode
        temp_tran = 0x1000;

        while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy

        //ethernet read data flow
        vSendString("Check auto Negotiation\n\r");
        recv_data = 0;

        while(recv_data & 0x20 != 0x20){
        *tran_data = 0x8210000; ///TEST

        while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
        while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
        *ethernet_wr_ready = 0;     //reset ready flag

        recv_data = *tran_data;
        }

        vSendString("Auto Negotiation COMPLETE\n\r");
//  vSendString("Ethernet LINKED\n\r");
    LINK_flag = 1;
    }

#endif

#if test_filter

    unsigned int filrt_enb_addr = 0x60009038;

    volatile unsigned int *filrt_enb = (unsigned int *) filrt_enb_addr;

    *filrt_enb = 1;

    unsigned int filrt_data1_addr = 0x60009030;

    volatile unsigned int *filrt_data1 = (unsigned int *) filrt_data1_addr;

    unsigned int filrt_data2_addr = 0x60009034;

    volatile unsigned int *filrt_data2 = (unsigned int *) filrt_data2_addr;

    *filrt_data1 = 0x35fd442c;
    *filrt_data2 = 0x925f;




#endif


    //wr addr

    unsigned int axi4_wr_addr = 0x60009000;

    volatile unsigned int *axi4_wr_data = (unsigned int *) axi4_wr_addr;

    unsigned int last_axi4_wr_addr = 0x60009004;

    volatile unsigned int *last_axi4_wr_data = (unsigned int *) last_axi4_wr_addr;


    //rev addr

    unsigned int start_axi4_rev_addr = 0x60009018;

    volatile unsigned int *start_axi4_rev_data = (unsigned int *) start_axi4_rev_addr;

    unsigned int axi4_rev_addr = 0x60009010;

    volatile unsigned int *axi4_rev_data = (unsigned int *) axi4_rev_addr;

    unsigned int last_axi4_rev_addr = 0x60009014;

    volatile unsigned int *last_axi4_rev_data = (unsigned int *) last_axi4_rev_addr;

    unsigned int axi4_status_addr = 0x60009020;

    volatile unsigned int *axi4_status_data = (unsigned int *) axi4_status_addr;

    //write program start
#if ping_DHCP
    int i,size,count;

        vSendString("Ethernet LINKED\n\r");
        vSendString("Send DHCP\n\r");

    delay(10);

    volatile unsigned int *PING_recv_data = (unsigned int *) (0x81020000);

    // first clean buffer
    while((*axi4_status_data & 0x1) == 0x1){
        CPC();
        memset(PING_recv_data, 0, 800);

        *axi4_rev_data = PING_recv_data;

        *start_axi4_rev_data = 0x1; // start DMA
        while((*axi4_status_data & 0x2) != 0x2);  // check DMA complete flag

        *axi4_status_data = *axi4_status_data | 0x2; // write bit[1] to 1 reset DMA flag
        size = *last_axi4_rev_data;
    }



    //block 1
    i = 0;
    volatile unsigned int *t_ram = (unsigned int *) (0x81000000 + 4*i);
    *t_ram = 0xffffffff;

    t_ram++;
    *t_ram = 0xe000ffff;

    //block 2


    t_ram++;
    *t_ram = 0x9d06684c;

    t_ram++;
    *t_ram = 0xc0450008;

    //block 3

    t_ram++;
    *t_ram = 0x00003e01;  // try dis

    t_ram++;
    *t_ram = 0x11400040;

    //block 4

    t_ram++;
    *t_ram = 0x0000f038;

    t_ram++;
    *t_ram = 0xffff0000;

    //block 5

    t_ram++;
    *t_ram = 0x4400ffff;

    t_ram++;
    *t_ram = 0x2a014300;

    //block 6

    t_ram++;
    *t_ram = 0x0101e707;

    t_ram++;
    *t_ram = 0xe4010006;

    //block 7

    t_ram++;
    *t_ram = 0x0100fe45;

    t_ram++;
    *t_ram = 0x0000ffff;//BC

    //block 8

    t_ram++;
    *t_ram = 0x00000000;//discover

    t_ram++;
    *t_ram = 0x00000000;

    //block 9

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0xe0000000;

    //block 10

    t_ram++;
    *t_ram = 0x9d06684c;

    t_ram++;
    *t_ram = 0x00000000;

    //block 11

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 12

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 13

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 14

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 15

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 16

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 17

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 18

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 19

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 20

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;
    //block 21

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 22

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 23

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 24

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 25

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 26

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 27

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 28

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 29

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 30

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;
    //block 31

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 32

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 33

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 34

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 35

    t_ram++;
    *t_ram = 0x00000000;


    t_ram++;
    *t_ram = 0x82630000;

    t_ram++;
    *t_ram = 0x01356353;

    t_ram++;
    *t_ram = 0x01073d01;

    t_ram++;
    *t_ram = 0x684ce000;

    t_ram++;
    *t_ram = 0x11379d06;

    t_ram++;
    *t_ram = 0x0c060201;

    t_ram++;
    *t_ram = 0x791c1a0f;

    t_ram++;
    *t_ram = 0x29282103;

    t_ram++;
    *t_ram = 0xfcf9772a;

    t_ram++;
    *t_ram = 0x02023911;

    t_ram++;
    *t_ram = 0xc0043240;

    t_ram++;
    *t_ram = 0x0c0350a8;

    t_ram++;
    *t_ram = 0x75627506;

    t_ram++;
    *t_ram = 0xff75746e;

    CPC();
    *axi4_wr_data = 0x81000000; // tran start addr

    // write the number of bytes


    *last_axi4_wr_data = 0x0000014C;

    unsigned int r_ram[200];

    while(*axi4_status_data & 0x4 != 0x4);
//  vSendString("SEND DHCP_DISCOVER DONE\n\r");


//  volatile unsigned int *check_dhcp = (unsigned int *) (0x82000028);
//  volatile unsigned int *check_reqack = (unsigned int *) (0x8200011c);
    //check request
    memset(r_ram, 0, 800);
    while(((r_ram[10] | 0x0000ffff) != 0x0102ffff) | ((r_ram[71] | 0xffffff00) != 0xffffff02)){

        while((*axi4_status_data & 0x1) == 0x1){

            CPC();

            *axi4_rev_data = r_ram;
//          *axi4_rev_data = 0x61030000;

            *start_axi4_rev_data = 0x1; // start DMA
            while((*axi4_status_data & 0x2) != 0x2);  // check DMA complete flag

            *axi4_status_data = *axi4_status_data | 0x2; // write bit[1] to 1 reset DMA flag
            size = *last_axi4_rev_data;

//          for(i = 0; i<=199;i++){
//          r_ram[i] = *(unsigned int *) (0x61030000+4*i);
//          }

        }
    }
    vSendString("\n\rOFFER RECV\n\r");
    unsigned int valid_ip = (r_ram[14] & 0xFFFF0000) | (r_ram[15] & 0x0000FFFF);

    // clean buffer
    while((*axi4_status_data & 0x1) == 0x1){
        CPC();
        *axi4_rev_data = PING_recv_data;

        *start_axi4_rev_data = 0x1; // start DMA
        while((*axi4_status_data & 0x2) == 0x2);  // check DMA complete flag

        *axi4_status_data = *axi4_status_data | 0x2; // write bit[1] to 1 reset DMA flag
        size = *last_axi4_rev_data;
    }



    int ip_ch = ((valid_ip & 0x0000FF00) >> 8);

    ip_ch = ip_ch - 2;

    //send request
    t_ram = 0x81000000;
    *t_ram = 0xffffffff;

    t_ram++;
    *t_ram = 0xe000ffff;

    //block 2

    t_ram++;
    *t_ram = 0x9d06684c;

    t_ram++;
    *t_ram = 0xc0450008;

    //block 3

    t_ram++;
    *t_ram = 0x00004401;  // try dis

    t_ram++;
    *t_ram = 0x11400040;

    //block 4

    t_ram++;
    *t_ram = 0x0000ea38;

    t_ram++;
    *t_ram = 0xffff0000;

    //block 5

    t_ram++;
    *t_ram = 0x4400ffff;

    t_ram++;
    *t_ram = 0x30014300;

    //block 6

    t_ram++;
    *t_ram = 0x01019458 - ip_ch;

    t_ram++;
    *t_ram = 0xe4010006;

    //block 7

    t_ram++;
    *t_ram = 0x0100fe45;

    t_ram++;
    *t_ram = 0x0000ffff;//BC

    //block 8

    t_ram++;
    *t_ram = 0x00000000;//discover

    t_ram++;
    *t_ram = 0x00000000;

    //block 9

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0xe0000000;

    //block 10

    t_ram++;
    *t_ram = 0x9d06684c;

    t_ram++;
    *t_ram = 0x00000000;

    //block 11

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 12

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 13

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 14

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 15

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 16

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 17

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 18

    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x00000000;

    //block 19
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 20
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;
    //block 21
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 22
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 23
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 24
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 25
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 26
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 27
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 28
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 29
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 30
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;
    //block 31
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 32
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 33
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 34
    t_ram++;
    *t_ram = 0x00000000;
    t_ram++;
    *t_ram = 0x00000000;

    //block 35
    t_ram++;
    *t_ram = 0x00000000;

    t_ram++;
    *t_ram = 0x82630000;
    t_ram++;
    *t_ram = 0x01356353;
    t_ram++;
    *t_ram = 0x01073d03;
    t_ram++;
    *t_ram = 0x684ce000;
    t_ram++;
    *t_ram = 0x11379d06;
    t_ram++;
    *t_ram = 0x0c060201;
    t_ram++;
    *t_ram = 0x791c1a0f;
    t_ram++;
    *t_ram = 0x29282103;
    t_ram++;
    *t_ram = 0xfcf9772a;
    t_ram++;
    *t_ram = 0x02023911;
    t_ram++;
    *t_ram = 0xc0043240;
    t_ram++;
    *t_ram = 0x360000a8 | ((valid_ip & 0x0000FFFF) << 8);
    t_ram++;
    *t_ram = 0x50a8c004;
    t_ram++;
    *t_ram = 0x75060c01;
    t_ram++;
    *t_ram = 0x746e7562;
    t_ram++;
    *t_ram = 0x0000ff75;

    CPC();

    *axi4_wr_data = 0x81000000; // tran start addr

    *last_axi4_wr_data = 0x00000152;

    while(*axi4_status_data & 0x4 != 0x4);
    vSendString("SEND DHCP_request DONE\n\r");
//check ACK
    memset(r_ram, 0, 800);
    while(((r_ram[10] | 0x0000ffff) != 0x0102ffff) | ((r_ram[71] | 0xffffff00) != 0xffffff05)){

        while((*axi4_status_data & 0x1) == 0x1){

            CPC();
            *axi4_rev_data = r_ram;
//          *axi4_rev_data = 0x61030000;

            *start_axi4_rev_data = 0x1; // start DMA
            while((*axi4_status_data & 0x2) != 0x2);  // check DMA complete flag

            *axi4_status_data = *axi4_status_data | 0x2; // write bit[1] to 1 reset DMA flag
            size = *last_axi4_rev_data;

//          for(i = 0; i<=199;i++){
//          r_ram[i] = *(unsigned int *) (0x61030000+4*i);
//          }

        }
    }

//  vSendString("\n\rACK RECV\n\r");
    char IIP[20];
    int ip_0, ip_1, ip_2, ip_3;
    ip_0 = ((valid_ip & 0xFF000000) >> 24);
    ip_1 = ((valid_ip & 0x00FF0000) >> 16);
    ip_2 = ((valid_ip & 0x0000FF00) >> 8);
    ip_3 = ((valid_ip & 0x000000FF));

    vSendString("\n\rACK RECV The IP given is :");
    int2str(IIP,ip_1);
    vSendString(IIP);
    vSendString(".");
    int2str(IIP,ip_0);
    vSendString(IIP);
    vSendString(".");
    int2str(IIP,ip_3);
    vSendString(IIP);
    vSendString(".");
    int2str(IIP,ip_2);
    vSendString(IIP);
    vSendString("\n\r");

#endif

#if check_ping
    vSendString("Ethernet LINKED\n\r");
    vSendString("REV PING\n\r");

    unsigned int temp1,temp2,dummy;

    while(1){
        memset(r_ram, 0, 800);


        if(*axi4_status_data & 0x1 == 0x1){

//          unsigned int r_ram[200];
            unsigned int tem[25];
            CPC();

            *axi4_rev_data = r_ram;
//          *axi4_rev_data = 0x61030000;

            *start_axi4_rev_data = 0x1; // start DMA
            while((*axi4_status_data & 0x2) != 0x2);  // check DMA complete flag

            *axi4_status_data = *axi4_status_data | 0x2; // write bit[1] to 1 reset DMA flag

            size = *last_axi4_rev_data;

//          for(i = 0; i<=199;i++){
//          r_ram[i] = *(unsigned int *) (0x61030000+4*i);
//          }
            if(size%4 == 0){
                count = size/4 +0;
            }else{
                count = size/4 +1;
            }


//          vSendString("\n\rPING DATA RECV END\n\r");

            if(r_ram[3] == 0x1000608){
//              vSendString("\n\r-----*******ARP DATA RECV*******-----\n\r");
                tem[0] = ((r_ram[1] & 0xffff0000)>> 16 ) | ((r_ram[2] & 0xFFFF) << 16);
                tem[1] = (((r_ram[2] & 0xffff0000)>> 16) | (0x442c0000));
                tem[2] = 0x925f35fd;
                tem[5] = ((r_ram[5]+0x100)&0xffff) | 0x442c0000;
                tem[6] = 0x925f35fd;
                tem[7] = ((r_ram[9] & 0xffff0000)>> 16 ) | ((r_ram[10] & 0xFFFF) << 16);
                tem[9] = ((r_ram[9] & 0xffff0000)) | ((r_ram[7] & 0xFFFF) << 16);
                tem[10] = ((r_ram[7] & 0xFFFF0000) >> 16);

                //block 1
                t_ram = 0x81000000;
                *t_ram = tem[0];
                t_ram++;
                *t_ram = tem[1];

                //block 2
                t_ram++;
                *t_ram = tem[2];
                t_ram++;
                *t_ram = r_ram[3];

                //block 3
                t_ram++;
                *t_ram = r_ram[4];
                t_ram++;
                *t_ram = tem[5];

                //block 4
                t_ram++;
                *t_ram = tem[6];
                t_ram++;
                *t_ram = tem[7];

                //block 5
                t_ram++;
                *t_ram = r_ram[8];
                t_ram++;
                *t_ram = tem[9];

                t_ram++;
                *t_ram = tem[10];

                //data block
                for(i=11;i<count;i++){

                    t_ram++;
                    *t_ram = r_ram[i];

                }

                CPC();

                *axi4_wr_data = 0x81000000; // tran start addr

                // write the number of bytes
                *last_axi4_wr_data = size;

                while(*axi4_status_data & 0x4 != 0x4);
//              vSendString("REPLY ARP DONE\n\r");


            }

            else if((r_ram[3] == 0x450008) & ((r_ram[5] & 0x11000000) == 0x01000000)){

//          vSendString("\n\r-----*******ICMP DATA RECV*******-----\n\r");
            tem[0] = ((r_ram[1] & 0xffff0000)>> 16 ) | ((r_ram[2] & 0xFFFF) << 16);
            tem[1] = ((r_ram[2] & 0xffff0000)>> 16) | ((r_ram[0] & 0xFFFF) << 16);
            tem[2] = ((r_ram[0] & 0xffff0000)>> 16 ) | ((r_ram[1] & 0xFFFF) << 16);

            tem[6] = ((r_ram[6]) & 0x0000ffff) | (r_ram[7] & 0xffff0000);
            tem[7] = (r_ram[6] & 0xffff0000) | (r_ram[8] & 0x0000ffff);
            tem[8] = (((r_ram[8] & 0xffff0000) - 0x80000)) | (r_ram[7] & 0x0000ffff);
            tem[9] = (r_ram[9] & 0xffff0000) | (~(~r_ram[9]-0x8) & 0x0000ffff);

//          vSendString("REPLY PING\n\r");
            //block 1
            t_ram = 0x81000000;
            *t_ram = tem[0];
            t_ram++;
            *t_ram = tem[1];

            //block 2
            t_ram++;
            *t_ram = tem[2];
            t_ram++;
            *t_ram = r_ram[3];

            //block 3
            t_ram++;
            *t_ram = r_ram[4];
            t_ram++;
            *t_ram = r_ram[5];

            //block 4
            t_ram++;
            *t_ram = tem[6];
            t_ram++;
            *t_ram = tem[7];

            //block 5
            t_ram++;
            *t_ram = tem[8];
            t_ram++;
            *t_ram = tem[9];    //corrected checksum

            //data block
            for(i=10;i<count;i++){

                t_ram++;
                *t_ram = r_ram[i];

            }
            CPC();

            *axi4_wr_data = 0x81000000; // tran start addr

            // write the number of bytes
            *last_axi4_wr_data = size;

            while(*axi4_status_data & 0x4 != 0x4);
//          vSendString("REPLY PING DONE\n\r");

        }}

        *tran_data = 0x8310000; ///TEST

        while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
        while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
        *ethernet_wr_ready = 0;     //reset ready flag

        recv_data = *tran_data;

        if((recv_data & 0x400) != 0x400){
            vSendString("Ethernet NOT LINKED\n\r");

        while((recv_data & 0x400) != 0x400){

        *tran_data = 0x8310000; ///TEST

        while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
        while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
        recv_data = *tran_data;
        *ethernet_wr_ready = 0;     //reset ready flag
        LINK_flag = 0;

    //  recv_data = *tran_data;
        }
        }else{}

        if(LINK_flag == 0){
            //ethernet tran data flow
            *tran_data = 0x4201000;     //enable PCS loopback mode   speed 100MHz   Enable Auto-Negotiation mode
            temp_tran = 0x1000;

            while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy

            //ethernet read data flow
            vSendString("Check auto Negotiation\n\r");
            recv_data = 0;

            while(recv_data & 0x20 != 0x20){
            *tran_data = 0x8210000; ///TEST

            while((*ethernet_wr_ready & 0x2) == 0x2);       //check busy
            while((*ethernet_wr_ready & 0x1) != 0x1);       //check received operation end
            *ethernet_wr_ready = 0;     //reset ready flag

            recv_data = *tran_data;
            }

            vSendString("Auto Negotiation COMPLETE\n\r");
        vSendString("Ethernet LINKED\n\r");
        LINK_flag = 1;
        }
    }

#endif

#if test_drop

    vSendString("Ethernet LINKED\n\r");
    vSendString("REV PING\n\r");
    int i,size,count;
    unsigned int temp1,temp2,dummy;

    delay(9999); //make ping start delay to test drop fame
    while(1){

        if(*axi4_status_data & 0x1 == 0x1 ){

            vSendString("PING DATA received\n\r");

//          dummy = *last_axi4_wr_data;
            size = *last_axi4_wr_data;
//          size = 98;
//          if(size == 98){
//              vSendString("CORRECT\n\r");
//          }
            if(size%4 == 0){
                count = size/4 +0;
            }else{
                count = size/4 +1;
            }

//          dummy = *axi4_wr_data;

            unsigned int ram[200];
            unsigned int tem[25];

            for(i=0;i<count;i++){
//          while(*axi4_status_data & 0x4 != 0x4){

                volatile unsigned int *PING_recv_data = (unsigned int *) (0x82000000 + 4*i);
                *PING_recv_data = *axi4_wr_data;
                ram[i] = *PING_recv_data;
//              temp1 = *axi4_wr_data;
//              temp2 = *axi4_wr_data;
//              *PING_recv_data = temp1;

            }

//          while(*axi4_status_data & 0x2 == 0x2);

            vSendString("\n\rPING DATA RECV END\n\r");

            if(ram[3] == 0x1000608){
                vSendString("\n\r-----*******ARP DATA RECV*******-----\n\r");
                tem[0] = ((ram[1] & 0xffff0000)>> 16 ) | ((ram[2] & 0xFFFF) << 16);
//              tem[0] = 0x35fd442c;
                tem[1] = (((ram[2] & 0xffff0000)>> 16) | (0x442c0000));
                tem[2] = 0x925f35fd;
                tem[5] = ((ram[5]+0x100)&0xffff) | 0x442c0000;
                tem[6] = 0x925f35fd;
                tem[7] = ((ram[9] & 0xffff0000)>> 16 ) | ((ram[10] & 0xFFFF) << 16);
                tem[9] = ((ram[9] & 0xffff0000)) | ((ram[7] & 0xFFFF) << 16);
                tem[10] = ((ram[7] & 0xFFFF0000) >> 16);

                //block 1
                *axi4_wr_data = tem[0];
                *axi4_wr_data = tem[1];

                //block 2
                *axi4_wr_data = tem[2];
                *axi4_wr_data = ram[3];

                //block 3
                *axi4_wr_data = ram[4];
                *axi4_wr_data = tem[5];

                //block 4
                *axi4_wr_data = tem[6];
                *axi4_wr_data = tem[7];

                //block 5
                *axi4_wr_data = ram[8];
                *axi4_wr_data = tem[9];

                *axi4_wr_data = tem[10];

//              size = 64;
//              count = 64/4;

                //data block
                for(i=11;i<count;i++){

                    *axi4_wr_data = ram[i];

                }

                // write the number of bytes
                *last_axi4_wr_data = size;

                while(*axi4_status_data & 0x4 != 0x4);
                vSendString("REPLY ARP DONE\n\r");


            }

            else if(ram[3] == 0x450008){

            vSendString("\n\r-----*******ICMP DATA RECV*******-----\n\r");
            tem[0] = ((ram[1] & 0xffff0000)>> 16 ) | ((ram[2] & 0xFFFF) << 16);
            tem[1] = ((ram[2] & 0xffff0000)>> 16) | ((ram[0] & 0xFFFF) << 16);
            tem[2] = ((ram[0] & 0xffff0000)>> 16 ) | ((ram[1] & 0xFFFF) << 16);

            tem[6] = ((ram[6]) & 0x0000ffff) | (ram[7] & 0xffff0000);
            tem[7] = (ram[6] & 0xffff0000) | (ram[8] & 0x0000ffff);
            tem[8] = (((ram[8] & 0xffff0000) - 0x80000)) | (ram[7] & 0x0000ffff);
            tem[9] = (ram[9] & 0xffff0000) | (~(~ram[9]-0x8) & 0x0000ffff);

            vSendString("REPLY PING\n\r");
            //block 1
            *axi4_wr_data = tem[0];
            *axi4_wr_data = tem[1];

            //block 2
            *axi4_wr_data = tem[2];
            *axi4_wr_data = ram[3];

            //block 3
            *axi4_wr_data = ram[4];
            *axi4_wr_data = ram[5];

            //block 4
            *axi4_wr_data = tem[6];
            *axi4_wr_data = tem[7];

            //block 5
            *axi4_wr_data = tem[8];
            *axi4_wr_data = tem[9]; //corrected checksum

            //data block
            for(i=10;i<count;i++){

                *axi4_wr_data = ram[i];

            }

            // write the number of bytes
            *last_axi4_wr_data = size;

            while(*axi4_status_data & 0x4 != 0x4);
            vSendString("REPLY PING DONE\n\r");


        }}


    }



#endif


    while(1);


}
