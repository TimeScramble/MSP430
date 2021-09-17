#include <msp430.h>
#include <stdint.h>


//global variables

volatile uint8_t comm[8];
volatile uint8_t puffer[10];
volatile uint8_t puffer_counter = 0;
volatile uint8_t jump = 0;

void i2c_write(uint8_t address, const uint8_t* data, uint8_t len)
{
    // Warten, bis Bus frei
    while (UCB0CTL1 & UCTXSTP)
    {
    }

    UCB0I2CSA = address; // Slave Adresse setzen
    UCB0CTL1 |= UCTR + UCTXSTT;      // W-Mode, Start der Übertragung

    // Senden der Datenbytes
    for (uint8_t i = 0; i < len; i++)
    {
        while (!(IFG2 & UCB0TXIFG))
        {
        }

        UCB0TXBUF = data[i];
    }

    // Senden der Stoppsequenz
    while (!(IFG2 & UCB0TXIFG))
    {
    }
    UCB0CTL1 |= UCTXSTP;
}

void i2c_read(uint8_t address, uint8_t* data, uint8_t len)
{
    // Warten, bis Bus frei
    while (UCB0CTL1 & UCTXSTP)
    {
    }

    UCB0I2CSA = address; // Slave Adresse setzen
    UCB0CTL1 &= ~UCTR;  // R-Mode
    UCB0CTL1 |= UCTXSTT; // Start der Übertragung

    // Warte, bis Startsequenz gesendet
    while (UCB0CTL1 & UCTXSTT)
    {
    }

    // Empfangen der Datenbytes
    for (uint8_t i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            // Setzen der Stop-Bits, während des Empfangen des letzten Bytes
            UCB0CTL1 |= UCTXSTP;
        }

        while (!(IFG2 & UCB0RXIFG))
        {
        }

        data[i] = UCB0RXBUF;
    }
}

void uart_print(char *str)
{
    while (*str != 0)
    {
        while (!(IFG2 & UCA0TXIFG))
        {
        }
        UCA0TXBUF = *str;
        *str++;
    }
}

void uart_hex(uint8_t number)
{
    char str[3];
    for (uint8_t i = 0; i < 2; i++)
    {
        uint8_t digit = number & 0xf;
        if (digit < 10)
        {
            str[1 - i] = '0' + digit;
        }
        else
        {
            str[1 - i] = 'a' + digit - 10;
        }
        number >>= 4;
    }

    str[2] = 0;
    uart_print(str);
}

int main(void)
{

    /*
     * Fragen:
     * Interrupt für UART-Empfangen fällt in eine Loop und hört nicht auf ... IFG2 wird aber eigentlich gelöscht
     * MSP430 hat zwei mögliche Interrputs : USCIAB0RX und USCIBB0RX ... kann evtl falscher Interrupt gelesen werden -> andere Interrupt wird nicht gelöscht und hängt sich auf?
     *
     */
    WDTCTL = WDTPW | WDTHOLD;    // stop watchdog timer

    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

/*
    P1DIR &= ~BIT3;
    P1OUT |= BIT3;
    P1REN |= BIT3;
    P1IES |= BIT3;
    P1IFG &= ~BIT3;
    P1IE |= BIT3;
*/

    // Pins
    P1SEL |= BIT1 + BIT2 + BIT6 + BIT7;
    P1SEL2 |= BIT1 + BIT2 + BIT6 + BIT7;

    //Interrupts

    // I²C
    UCB0CTL1 |= UCSWRST;                // Soft-Reset des I2C-Moduls
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;   // Master, I2C, Synchron
    UCB0BR0 = 20;   // Teiler: 8 MHz / 20 = 400 kHz
    UCB0BR1 = 0;
    UCB0CTL1 = UCSSEL_2; // Reset-Loslassen, SMCLK

    // UART
    UCA0CTL1 |= UCSWRST;
    UCA0CTL0 = 0;
    UCA0CTL1 |= UCSSEL_2;
    UCA0BR0 = 104;
    UCA0BR1 = 0;
    UCA0MCTL = (1 << 1);
    UCA0CTL1 &= ~UCSWRST;

    //test section
    UC0IE |= UCA0RXIE;      //Interrupt für Datenempfang
    //test section

    __enable_interrupt();

    uint8_t buff[8];


    // ID Test
    //uart_print("ID (0x92) = ");
    buff[0] = 0x92;
    i2c_write(0x39, buff, 1);
    i2c_read(0x39, buff, 1);
    uart_hex(buff[0]);
    uart_print("\r\n");

    // CONFIG WTIME
    buff[0] = 0x8D;
    buff[1] = 0b01100010;       //activating WLONG
    i2c_write(0x39, buff, 2);

    // WTIME
    buff[0] = 0x83;
    buff[1] = 0xFF;             //wtime increase
    i2c_write(0x39, buff, 2);


    // PON
    buff[0] = 0x80;
    buff[1] = BIT3 + BIT2 + BIT1 + BIT0;
    comm [2] = buff[1];             //saving PON settings
    i2c_write(0x39, buff, 2);


    // CONTROL
    buff[0] = 0x8F;
    buff[1] = BIT7 + BIT6 + BIT3 + BIT2 + BIT1 + BIT0;
    i2c_write(0x39, buff, 2);

    while (1)
    {


        if ( puffer_counter != 0 ){
            buff[0] = comm[0];
            buff[1] = puffer[puffer_counter];

            i2c_write(0x39, buff, 2);
            uart_print("GGcommand was written\n");
            puffer_counter--;
        }



        //CONTROL
 /*       for (uint8_t i = counter; i > 0 ; i-- ){
            switch (jump){

                case 1:
                    comm[0] = 0x80;
                    comm[1] = comm[2];  //
                    break;
                default:
                    comm[0] = 0x8F;
                    break;
            }
            i2c_write(0x39, comm, 1);
            uart_print("for-loop entered\r\n");
        }

        counter = 0;
*/

        // Proximity
        uart_print("PDATA = ");
        buff[0] = 0x9C;
        i2c_write(0x39, buff, 1);
        i2c_read(0x39, buff, 1);
        uart_hex(buff[0]);

        // COLOR
        buff[0] = 0x94;
        i2c_write(0x39, buff, 1);
        i2c_read(0x39, buff, 8);

        uart_print("  CDATA = ");
        uart_hex(buff[1]);
        uart_hex(buff[0]);

        uart_print("  RDATA = ");
        uart_hex(buff[3]);
        uart_hex(buff[2]);

        uart_print("  GDATA = ");
        uart_hex(buff[5]);
        uart_hex(buff[4]);

        uart_print("  BDATA = ");
        uart_hex(buff[7]);
        uart_hex(buff[6]);

        uart_print("\r\n");

        __delay_cycles(100000L);
    }
}


#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {


    if (IFG2 & UCA0RXIFG) {
        IFG2 &= ~UCA0RXIFG;   //reset IFG-register

        //IFG2 = IFG2 & 0x0A;     //hoffentlich werden nun wirklich alle flags auf 0 gesetzt
        comm[0] = 0x8F;
        puffer[puffer_counter] = UCA0RXBUF;

        if (puffer_counter < 10 ){
            puffer_counter++;
        }
        else{
            puffer_counter = 0; //Überschreiben der als erstes gesendeten Daten ... wenn die noch nicht abgearbeitet sind, sind sie auch nicht mehr von Interesse
        }

        //uart_print("GGcommand written\n\n");

    }
}


/*
 * Possible solution :
 *
        switch (received_int) {

        //default buff[1] = BIT7 + BIT6 + BIT3 + BIT2 + BIT1 + BIT0;
        case '1':
            comm[0]=0x8F;
            comm[1] = BIT7 + BIT6 + BIT3 + BIT2 + BIT0 ;        //gain : 4x
           // uart_print("11gain changed to 4x \r\n");
            break;
        case '2':
            comm[0]=0x8F;
            comm[1] = BIT7 + BIT6 + BIT3 + BIT2 + BIT1 ;        //gain : 16x

            break;
        case '3':
            comm[0]=0x8F;
            comm[1] = BIT7 + BIT6 + BIT3 + BIT2 + BIT1 + BIT0;  //gain : 64x

            break;
        case 'f':
            comm[0]=0x83;
            comm[1] = 0xFF;                                     //increase
            break;
        case 's':
            comm[0]=0x83;
            comm[1] = 0x00;                                     //decrease
            break;
        default:
            comm[0]=0x8F;
            comm[1] = BIT7 + BIT6 + BIT3 + BIT2;                //gain : 1x

            break;

        }

*/

       //comm[0]=0x8F;
       //i2c_write(0x39, comm, 1);

       //jump = 0;
       //counter++;
       //uart_hex(counter);


/*
 * End of possbile solution
*/


/*
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR() {
    if (P1IFG & BIT3) {
        P1IFG &= ~BIT3;

        comm[2] ^= BIT0;        //toggeln POWER ON

        jump = 1;
        counter ++;
    }
}
*/
