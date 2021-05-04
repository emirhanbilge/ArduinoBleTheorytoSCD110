#include <ArduinoBLE.h> // BlE için
#include <stdint.h>     // Bytleların okunabilir değerlere dönüşümü için
//#include <CurieBLE.h> Bu kütüphanede kullanılabilir
/// Burada little endian mı big endian mı dokümanda belirtilmediği için 2 fonksyonu da aldım fakat big endian kullanacağım
int16_t be16_to_cpu_signed(const uint8_t data[static 2])
{
    uint32_t val = (((uint32_t)data[0]) << 8) |
                   (((uint32_t)data[1]) << 0);
    return ((int32_t)val) - 0x10000u;
}
/*
int16_t le16_to_cpu_signed(const uint8_t data[static 2])
{
    uint32_t val = (((uint32_t)data[0]) << 0) |
                   (((uint32_t)data[1]) << 8);
    return ((int32_t)val) - 0x10000u;
}*/

// aşağıdaki değerler de 32'likler içindir çünkü 2 veri türü dönüşü DHT110da bulunmakta structların içinde bu daha net görülmektedir.
float ieee_float(uint32_t f)
{
    static_assert(sizeof(float) == sizeof f, "`float` has a weird size.");
    float ret;
    std::memcpy(&ret, &f, sizeof(float));
    return ret;
}

typedef unsigned char BYTE;

Accel accelaration;
Temperature temprature;
Magnetometer magnometer;

void bitParser(byte arr[])
{

    //// Bu fonksiyonda 33 bytle'lık verinin bölümlere ayrılık bitlerin doğru ve oluşturduğum struct objelerine aktarıyorum
    accelaration->ArithmMean_x[0] << arr[0];
    accelaration->ArithmMean_x[1] << arr[1];

    accelaration->ArithmMean_y[0] << arr[2];
    accelaration->ArithmMean_y[1] << arr[3];

    accelaration->ArithmMean_x[0] << arr[4];
    accelaration->ArithmMean_x[1] << arr[5];

    accelaration->Variance_x[0] << arr[6];
    accelaration->Variance_x[1] << arr[7];
    accelaration->Variance_x[2] << arr[8];
    accelaration->Variance_x[3] << arr[9];

    accelaration->Variance_y[0] << arr[10];
    accelaration->Variance_y[1] << arr[11];
    accelaration->Variance_y[2] << arr[12];
    accelaration->Variance_y[3] << arr[13];

    accelaration->Variance_z[0] << arr[14];
    accelaration->Variance_z[1] << arr[15];
    accelaration->Variance_z[2] << arr[16];
    accelaration->Variance_z[3] << arr[17];

    temprature->RawValue[0] << arr[18];
    temprature->RawValue[1] << arr[19];

    magnometer->Raw_x[0] << arr[24];
    magnometer->Raw_x[1] << arr[25];

    magnometer->Raw_y[0] << arr[26];
    magnometer->Raw_y[1] << arr[27];

    magnometer->Raw_x[0] << arr[28];
    magnometer->Raw_x[1] << arr[29];

    // Bu işlemin sonunda elimizde raw input yani ham data var fakat bunların türlerinin dönüşümünün yapılması unutulmamalıdır.
}

struct Accel // Dökümanda 6 değişkeni olduğu söylenmekte
{
    int16_t ArithmMean_x;
    int16_t ArithmMean_y;
    int16_t ArithmMean_z;

    uint32_t Variance_x;
    uint32_t Variance_y;
    uint32_t Variance_z;
};

struct Temperature // 2 bytlelık bir değer
{
    int16_t RawValue;
};

struct Magnetometer // Dökümandaki değişken adlarının birebir aynısı ve türün de aynısı kullanılmıştır.
{
    int16_t Raw_x;
    int16_t Raw_y;
    int16_t Raw_z;
};

void setup()
{
    Serial.Begin(9600);
}

void loop()
{

    if (!BLE.begin())
    {
        Serial.println("starting BLE failed!");
        while (1)
            ;
    }

    Serial.println("BLE Central scan");
    const BYTE anormal[33]; // SCD Characteristic 'STE Results' 33 byte result returning 33 Byte unsigned çünkü tür dönüşümünü biz yapacağız
    BLE.scan();
    BLEDevice peripheral = BLE.available(); // SCD 110 Periphel cihaz olarak belirli aralıklarla yayın yapar Central biziz ya da data talep eden

    if (peripheral) // bağlanabilinecek cihazlar ya da veri akışı yapan cihaz bulunmuşsa true bulunamamışsa false olucaktır
        Serial.println("Connecting ...");

    if (peripheral.connect()) // cihaza bağlanma veri alma
    {
        Serial.println("Connected");
    }
    else
    {
        Serial.println("Failed to connect!");
        return;
    }

    /*

    Dökümantasyona bakıldığı zaman verileri almamız için '3.1 SCD Universally Unique Identifiers (UUIDs)' başlığı altında : SCD registers BLE services statically.SCD 
    services are always discoverable even in mode selection(default mode).In default mode the services are not containing real data.After the Characteristic 'Mode 
    Selection' is set to a specific mode, the services will be enabled to supply real data.şeklinde bir bilgilendirme var.Burada Characteristic modu değiştirmemizi 
    ve modu seçmemiz gerektiğini anladım.Gerçek datalar için !!!' Service Short Term Experiment' servisinde 'Characteristic STE Results' characteristicini seçmemiz 
    gerektiğini görüyoruz.!!!STE Results 'a baktığımızda "Characteristic STE Results" başlığı altında '(Bosch dökümanı sayfa 20)33 byte 'lık bir veri dönüşü 
    sağlanmakta Ve bu 33 bytle' lik verinin hangi bytlelarının hangi sensör verisini içerdiği ise 20 -21. sayfalar arasında bir tabloda verilmiş olarak bulunmakta.
    */
    BLEService ShortTermExperiment = peripheral.service("02a65821-1000-1000-2000-b05cb05cb05c"); // short term ud , Dökümanda verilen 'short term' servisinin ayarlanması
    // Dökümanda Servicelerin ve uud , name ve daha detaylı tabloların verildiği yerden aldım sayfa (7-8) -8. sayfanın ilk row'unda -

    if (ShortTermExperiment) // Bu Servisin varlığı doğrulanırsa true , aksi takdirde false döneceğinden onun kontrolü yapılıyor.
    {
        // use the Servisin shortTermService olması lazım datayı almak için
        /// Short Term Servisinin altında 2 tane characteristic vardır bunlardan biz Characteristic 'STE Results' olanı seçmemiz gerekmekte dökümanda 8.sayfada bunun da id'si
        // Verilmiş olarak bulunmakta alttaki kodda o characteristiği ayarlıyoruz
        BLECharacteristic ShortTermExperiment_Reslt = peripheral.characteristic("02a65821-1002-1000-2000-b05cb05cb05c"); // verilen olduğu karakterislik

        if (ShortTermExperiment_Reslt) // Eğer bu characteristic de sağlandıysa ve bulunmaktaysa diye kontrol ediyorum.
        {
            memcpy(anormal, ShortTermExperiment_Reslt.read(), 35) // 33 byte'lik arreye de kopyalayabilirdim datayı fakat boş bytleler null ile işaretlenicek
                bitParser(anormal);                               // bu bytleleri dökümantasyona göre verileri düzenlemek için bu fonksyonuma yolluyorum
            delay(100);
            printDatas();
        }
        else
        {
            Serial.println("Peripheral does NOT have  characteristic");
        }
    }
    else
    {
        Serial.println("Peripheral does NOT have  service");
    }
}

void printDatas()
{

    // ! birinin 16 bit diğerinin 32 bit olduğundan dolayı fonksyonlar farklıdır.
    Serial.print("X : ", be16_to_cpu_signed(accelaration->ArithmMean_x), " Y : ", be16_to_cpu_signed(accelaration->ArithmMean_y), " Z : ", be16_to_cpu_signed(accelaration->ArithmMean_z));
    Serial.print("X Var : ", ieee_float(accelaration->Variance_x), "Y Var : ", ieee_float(accelaration->Variance_y), "Z Var : ", ieee_float(accelaration->Variance_z));

    Serial.print("Tempareture : ", be16_to_cpu_signed(temprature->RawValue)); // 16 bit ısı değeri

    // 16 bitlik mag. değerleri
    Serial.print("Mag X : ", be16_to_cpu_signed(magnometer->Raw_x), "Mag Y : ", be16_to_cpu_signed(magnometer->Raw_y), "Mag Z : ", be16_to_cpu_signed(magnometer->Raw_z));
}

/* References : 
https://www.arduino.cc/en/Reference/CurieBLE
https://www.arduino.cc/en/Reference/ArduinoBLE
https://stackoverflow.com/questions/57319676/proper-modern-way-of-converting-two-uint8-t-into-int16-t
https://stackoverflow.com/questions/48803363/bitwise-casting-uint32-t-to-float-in-c-c
https://www.bosch-connectivity.com/media/product_detail_scd/scd-ble-communication-protocol.pdf
https://forum.arduino.cc/t/how-to-connect-with-one-master-to-2-slaves/670681
https://www.novelbits.io/bluetooth-low-energy-advertisements-part-1/
https://www.techonthenet.com/c_language/standard_library_functions/string_h/memcpy.php#:~:text=In%20the%20C%20Programming%20Language,work%20if%20the%20objects%20overlap.
https://github.com/f32c/arduino/blob/master/hardware/fpga/f32c/system/include/stdint.h !!! arduino bit dönüşümleri için


Kullanılabilecek kartlar : 
ARDUINO NANO 33 BLE 
Beetle BLE - Arduino Bluetooth 4.0 Modül (BLE)
Bluefruit Bluetooth - UART Modül (BLE)