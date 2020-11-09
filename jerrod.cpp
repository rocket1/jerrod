#include <iostream>
#include <fstream>
#include <vector>

// G L O B A L S (declared static so they are only visible in this file) ...

// FIELD_SZ is the FIXED length for a "field" (e.g. First Name).
// E.g. "Jason" occupies 5 "letters" followed by 251 NULLs (The '\0' character).
static const u_int FIELD_SZ(256);
static const u_int MAX_CONTACTS(5);

// This is the file that is saved to disk.
// It holds all the contacts in bonary (not human-readable directly).
static const std::string gContactsDB( "./contacts.db" );

// These are our streams for reading and writing.
static std::ifstream gIn;
static std::ofstream gOut;

// A Contact (declared here, defined later) is a contact "record".
// Its EXACT size is: number of fields * size of each field (i.e. FIELD_SZ which is 256).
struct Contact; // Forward declaration.
static std::vector<Contact> gContacts;

// Just some handy constants for printing the field labels.
static const std::string gFirstNameLabel( "First Name" );
static const std::string gLastNameLabel( "Last Name" );
static const std::string gCountryLabel( "Country" );
static const std::string gStateLabel( "State" );
static const std::string gAddress1Label( "Address Line 1" );
static const std::string gAddress2Label( "Address Line 2" );
static const std::string gZipLabel( "Zip" );
static const std::string gHomePhoneLabel( "Home Phone" );
static const std::string gWorkPhoneLabel( "Work Phone" );

// The line above below each header that we print.
static const std::string gLine( "\n---------------------------------------------\n" );

// ... E N D  G L O B A L S

// A struct is just a handy way to group relate fields.
struct Contact
{
  char id[FIELD_SZ];
  char first_name[FIELD_SZ];
  char last_name[FIELD_SZ];
  char country[FIELD_SZ];
  char state[FIELD_SZ];
  char address_1[FIELD_SZ];
  char address_2[FIELD_SZ];
  char zip[FIELD_SZ];
  char home_phone[FIELD_SZ];
  char work_phone[FIELD_SZ];
};

// This function makes the command-line input nicer...
// Don't worry too much about it.  cin acts goofy sometimes on the command-line.
static void
fix_cin()
{
  std::cin.clear();
  std::cin.ignore( INT_MAX, '\n' );
}

// Display the "Choice ?" prompt and return the user's choice.
static const
u_int prompt_choice( const u_int min, const u_int max )
{
  u_int choice;
  
  while ( choice < min || choice > max ) {
    std::cout << "\n\nChoice? ";
    std::cin >> choice;
    fix_cin();
  }
  std::cout << "\n";
  return choice;
}

// The next three functions are formatting and print utilities.
static void
hdr( const std::string& title )
{
  std::cout << title << gLine;
}

// Print a message...
static void
msg( const std::string& text )
{
  std::cout << "\n" << text << "\n";
}

// Print ONE field from a Contact (e.g. First Name: Jerrod)
static void
dump_field( const std::string& label, const std::string& value )
{
  std::cout << label << ": " << value << "\n";
}

static void
dump_contact( const Contact& contact )
{
  hdr( "Contact Info" );

  dump_field( gFirstNameLabel, contact.first_name );
  dump_field( gLastNameLabel,  contact.last_name );
  dump_field( gCountryLabel,   contact.country );
  dump_field( gStateLabel,     contact.state );
  dump_field( gAddress1Label,  contact.address_1 );
  dump_field( gAddress2Label,  contact.address_2 );
  dump_field( gZipLabel,       contact.zip );
  dump_field( gHomePhoneLabel, contact.home_phone );
  dump_field( gWorkPhoneLabel, contact.work_phone );
}

// This dumps an enumerated list of Contacts.
static bool
dump_contact_list()
{ 
  if ( gContacts.empty() ) {
    std::cout << "[No Contacts Found]\n";
    return false;
  }

  for ( u_int i = 0; i < gContacts.size(); ++i ) {
    if ( i > 0 ) {
      std::cout << "\n"; // Formatting.
    }
    Contact& c = gContacts[i];
    std::cout << (i + 1) << ")  " << c.last_name << ", " << c.first_name;
  }

  return true;
}

// Read all the contacts from the "contacts.db" file.
static bool
read_all_contacts()
{
  gContacts.clear();
  gIn.open( gContactsDB.c_str(), std::ios::binary );
  
  if ( gIn.fail() ) {
    msg( "Failed reading contacts." );
    return false;
  }
  
  while (1) {

    // Create a new empty contact.  We're going to fill the space allocated in memory
    // for it with bytes from a file (2560 Bytes to be exact).
    Contact contact;

    // This says: read 2560 bytes from the file (contacts.db) into
    // memory, and put it on top of whatever is at the address of the variable contact (i.e. &contact).
    // What this effectively does is initialize our new contact struct with the data for one contact in the file.
    // The file pointer moves automatically to the end of the block in the file
    // so we're ready to get the next 2560 bytes into another Contact struct.
    //
    // The reason why this magically works is because we have very strict constraints
    // on the field size (256) * 10 fields which means that everything is guarenteed to line up correctly.
    // The reinterpret_cast is because std::ifstream::read expect a pointer to a character (char *),
    // Ultimately the incoming bytes end of being of type Contact because
    // that's what the type system think the variable "contact" is.
    // It just makes the read function happy and compile.

    gIn.read( reinterpret_cast<char*>(&contact), sizeof contact );

    if ( gIn.eof() ) {
      break;
    }

    gContacts.push_back( contact );
    std::cout << "Read contact \"" << std::string(contact.last_name) << ", " << std::string(contact.first_name) << "\".\n";
  }

  gIn.close();
  return true;
}

static bool
write_all_contacts()
{
  // Everytime we add a new contact, we re-write the disk file.
  gOut.open( gContactsDB.c_str(), std::ios::binary | std::ios::out );
  
  if ( gOut.fail() ) {
    msg( "Failed writing contacts." );
    return false;
  }
  
  for ( u_int i = 0; i < gContacts.size(); ++i ) {

    Contact& c = gContacts[i];

    // This just says write all the bytes from c to disk.
    // write needs char* as first arg, hence reinterpret_cast<char *>
    gOut.write( reinterpret_cast<char*>(&c), sizeof c );
  }

  gOut.close();
}

// This handles adding/editing of one Contact field.
static void
prompt_field( const std::string& label,
const Contact& contact,
char* prev_val,
bool do_edit )
{
  bool bad_input = false;

  do {
    
    if (do_edit) {
      
      std::cout << "Edit "
      << label
      << "  (default: "
      << (prev_val[0] != '\0' ? prev_val : "<empty>")
      << ")";
    }
    else {
      std::cout << label;
    }
    
    std::cout << ": ";
    std::string buf;
    std::getline( std::cin, buf );
    
    if ( !std::cin ) {
      bad_input = true;
      fix_cin();
    }

    // If we are in "edit" mode (i.e. do_edit is true),
    // AND the buf  is empty (i.e. user pressed return without input),
    // then just continue in the while loop (i.e. skip the strcpy stuff below).

    if ( do_edit && buf.empty() ) {
      // Keep saved value.
      continue;
    }

    // We passed in a char* (i.e. a pointer-to versus a copy-of the previous value)
    // to prev_val, so we can edit it directly.
    // See the param list above for the char* prev_val.
    // First initialize it to all NULL...
    ::memset( prev_val, '\0', FIELD_SZ );

    // Then copy in the user inputted data into the prev_val (i.e. overwrite it).
    strcpy( prev_val, buf.c_str() );
    return;

  } while (bad_input);
}

// This function prompts for each field (add or edit).
static bool
add_or_edit( Contact& contact, bool do_edit )
{
  prompt_field( gFirstNameLabel, contact, contact.first_name, do_edit );
  prompt_field( gLastNameLabel,  contact, contact.last_name,  do_edit );
  prompt_field( gCountryLabel,   contact, contact.country,    do_edit );
  prompt_field( gStateLabel,     contact, contact.state,      do_edit );
  prompt_field( gAddress1Label,  contact, contact.address_1,  do_edit );
  prompt_field( gAddress2Label,  contact, contact.address_2,  do_edit );
  prompt_field( gZipLabel,       contact, contact.zip,        do_edit );
  prompt_field( gHomePhoneLabel, contact, contact.home_phone, do_edit );
  prompt_field( gWorkPhoneLabel, contact, contact.work_phone, do_edit );
}

static bool
add()
{
  hdr( "Add New Contact" );

  if ( gContacts.size() >= MAX_CONTACTS ) {
    std::cout << "[Maximum Contacts Reached]\n";
    return false;
  }
  
  // Create a new, empty, Contact.
  Contact contact;

  // Initialize the entire 2560 Bytes of the new contact
  // structure with the NULL character (i.e. '\0').
  ::memset( &contact, '\0', sizeof contact );

  // Prompt for each field.  The "false" means we're not editing.
  add_or_edit( contact, false );

  // Add this new contact to our new master global list.
  gContacts.push_back( contact );

  // Write this list to disk.
  write_all_contacts();

  msg( "Contact added." );
  return true;
}

static bool
edit()
{
  hdr( "Edit Contact" );

  if ( !dump_contact_list() ) {
    return false;
  }
  
  u_int choice = prompt_choice( 1, gContacts.size() );
  
  hdr( "Press <ENTER> to keep default." );
  add_or_edit( gContacts[ choice - 1 ], true );
  write_all_contacts();
  msg( "Contact Info Updated." );
  return true;
}

static bool
find()
{
  hdr( "Find Contact" );

  if ( !dump_contact_list() ) {
    return false;
  }

  u_int choice = prompt_choice( 1, gContacts.size() );

  // Array index one less than menu's enumeration.
  // I.e. the user selects "1", that is index 0 in the contacts array.
  dump_contact( gContacts[ choice - 1 ] ); 
  return true;
}

static void
menu()
{
  while (1) {

    hdr( "\nContacts Database Menu" );

    std::cout << "1) Add New\n"
    << "2) Edit Saved Contact\n"
    << "3) Find\n"
    << "4) Load from Disk\n"
    << "5) Quit";
    
    u_int choice = prompt_choice( 1, 5 );

    switch (choice) {
    case 1:
      add();
      break;
    case 2:
      edit();
      break;
    case 3:
      find();
      break;
    case 4:
      read_all_contacts();
      break;
    case 5:
      std::cout << "Goodbye.\n";
      exit(0);
    default:
      continue;
    }
  }
}

int
main( int argc, char** argv )
{
  menu();
  return 0;
}
