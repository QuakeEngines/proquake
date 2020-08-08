#include <windows.h>

extern "C"{

void CreateSetKeyExtension(void);
void CreateSetKeyDescription(void);
void CreateSetKeyCommandLine(const char*  exeline);

};

/*void WriteRegTestObs(void)
{
 // Ok, we create a directory and store an integer and a string..
 unsigned long IsNew = 0;
 // RegCreateKeyEx will return a value that idicates if we created the dir or
 // or it was already exists...
 // values: (REG_CREATED_NEW_KEY)1 = just created, (REG_OPENED_EXISTING_KEY) 2 = exists
 HKEY hregkey;
 // Here, we create a directory called: "TESTING"
 long res = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\TESTING",
                           NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);
 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, "Hello Registry");
  RegSetValueEx(hregkey, "String", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  // Now the number
  int num = 128;
  RegSetValueEx(hregkey, "Number", 0, REG_DWORD, (const unsigned char *)&num, sizeof(num));
  //Don't forget to close the key...
  RegCloseKey(hregkey);
 }
 //Now let's read what we wrote, in order to see it really works :)
 char temp[256];
 int getnum = 0;

 res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\TESTING", 0, KEY_ALL_ACCESS, &hregkey);
 //Notice we now call RegOPENKeyEx...

 if (res == 0)
 {
  // Let's read the string first
  int datasize = 256; // The maximum length of temp;
  RegQueryValueEx(hregkey, "String",
                  NULL, NULL, (unsigned char *)&temp, (unsigned long *)&datasize);
  MessageBox(NULL, temp, "We got the string:", MB_OK);
 
  // Now the number
  datasize = sizeof(int); // The maximum length of getnum
  RegQueryValueEx(hregkey, "Number",
                  NULL, NULL, (unsigned char *)&getnum, (unsigned long *)&datasize);
  itoa(getnum, temp, 10);
  MessageBox(NULL, temp, "We got the number:", MB_OK);

  //Now let's delete the Number item
  RegDeleteValue(hregkey, "Number");

  //Now let's delete the directory we created, "TESTING"
  RegDeleteKey(HKEY_LOCAL_MACHINE, "Software\\TESTING");

  // Well we don't need to call RegCloseKey 'cause the key is deleted already..
  // But if you don't delete the key don't forget to call it

  // RegCloseKey(hregkey);
 }*/
 
void CreateSetKeyExtension(void)
{
 
 unsigned long IsNew = 0;
 HKEY hregkey;

 // CreateNewKey: REG_CREATED_NEW_KEY = just created, REG_OPENED_EXISTING_KEY = exists
 
// REG_OPENED_EXISTING_KEY
// long res = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\TESTING",
//                           NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, ".dem", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, "Quake");
  //RegSetValueEx(hregkey, "String", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
 
  RegCloseKey(hregkey);
 }

}

void CreateSetKeyDescription(void)
{
 
 unsigned long IsNew = 0;
 HKEY hregkey;

 // CreateNewKey: REG_CREATED_NEW_KEY = just created, REG_OPENED_EXISTING_KEY = exists
 
// REG_OPENED_EXISTING_KEY
// long res = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\TESTING",
//                           NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, "Quake", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, "Quake Demo");
  //RegSetValueEx(hregkey, "String", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
 
  RegCloseKey(hregkey);
 }

}



void CreateSetKeyCommandLine(const char* exeline)
{
	// Must send something like c:\quake\quake.exe %1
 unsigned long IsNew = 0;
 HKEY hregkey;

 // CreateNewKey: REG_CREATED_NEW_KEY = just created, REG_OPENED_EXISTING_KEY = exists
 
// REG_OPENED_EXISTING_KEY
// long res = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\TESTING",
//                           NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 long res = RegCreateKeyEx(HKEY_CLASSES_ROOT, "Quake\\shell\\open\\command", 
						   NULL, NULL, NULL, KEY_WRITE, NULL, &hregkey, &IsNew);

 if (res == 0) // If we created it successfully
 {
  // Storing the string
  char str[256];
  strcpy(str, exeline);
//  str += ' %1'; 
  //sprintf(str, "%s %1", exename);
  //RegSetValueEx(hregkey, "String", 0, REG_SZ, (const unsigned char *)str, strlen(str));
  RegSetValueEx(hregkey, "", 0, REG_SZ, (const unsigned char *)str, strlen(str));
//  MessageBox(NULL, str, "We got the number:", MB_OK);
  RegCloseKey(hregkey);
 }

}

