#include <iostream>
#include <string>
#include <stack>
using namespace std;

// Struktur untuk menyimpan aksi yang dilakukan oleh pengguna
struct Action {
    // Jenis aksi yang bisa dilakukan
    enum ActionType { 
        INSERT_LINE,    // Menyisipkan baris baru
        DELETE_LINE,    // Menghapus baris
        INSERT_CHAR,    // Menyisipkan karakter
        DELETE_CHAR,    // Menghapus karakter
        REPLACE_CHAR    // Mengganti karakter
    } type;

    int linePosition;    // Posisi baris dalam linked list (dimulai dari 0)
    int charIndex;       // Indeks karakter dalam baris (hanya untuk aksi karakter)
    string data;         // Data yang terkait dengan aksi (untuk INSERT_LINE dan DELETE_LINE)
    char oldChar;        // Karakter lama sebelum diganti (untuk REPLACE_CHAR)
    char newChar;        // Karakter baru setelah diganti (untuk REPLACE_CHAR)

    // Konstruktor untuk aksi INSERT_LINE dan DELETE_LINE
    Action(ActionType type, int linePos, const string& d)
        : type(type), linePosition(linePos), charIndex(-1), data(d), oldChar('\0'), newChar('\0') {}

    // Konstruktor untuk aksi karakter: INSERT_CHAR, DELETE_CHAR, REPLACE_CHAR
    Action(ActionType type, int linePos, int cIndex, char oldC = '\0', char newC = '\0')
        : type(type), linePosition(linePos), charIndex(cIndex), data(""), oldChar(oldC), newChar(newC) {}
};

// Kelas Node merepresentasikan satu baris teks dalam editor
class Node {
public:
    string data; // Teks yang disimpan dalam baris ini
    Node* prev;  // Pointer ke node (baris) sebelumnya
    Node* next;  // Pointer ke node (baris) berikutnya

    // Konstruktor untuk Node
    Node(string data) {
        this->data = data;
        this->prev = nullptr;
        this->next = nullptr;
    }
};

// Kelas LinkedList mengelola daftar baris teks dan operasi terkait
class LinkedList {
private:
    Node* head; // Pointer ke baris pertama dalam linked list
    Node* tail; // Pointer ke baris terakhir dalam linked list

    // Stack untuk menyimpan aksi yang bisa di-undo dan di-redo
    stack<Action> undoStack; // Menyimpan aksi untuk undo
    stack<Action> redoStack; // Menyimpan aksi untuk redo

    // Untuk navigasi dan penyorotan (highlighting)
    Node* currentNode;        // Baris (node) yang sedang di-highlight
    int currentCharIndex;     // Indeks karakter yang sedang di-highlight dalam currentNode

    // Fungsi untuk mendapatkan posisi baris saat ini dalam linked list
    int getCurrentLinePosition() {
        int pos = 0;
        Node* temp = head;
        while (temp != nullptr && temp != currentNode) {
            temp = temp->next;
            pos++;
        }
        return (temp == currentNode) ? pos : -1;
    }

public:
    // Konstruktor untuk LinkedList
    LinkedList() {
        head = nullptr;
        tail = nullptr;
        currentNode = nullptr;
        currentCharIndex = 0;
    }

    // Destruktor untuk membersihkan memori yang dialokasikan
    ~LinkedList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current; // Menghapus node saat ini
            current = next; // Pindah ke node berikutnya
        }
    }

    // Fungsi untuk menyisipkan baris baru pada posisi tertentu
    void insertLine(int position, const string& data, bool record = true) {
        Node* newNode = new Node(data); // Membuat node baru dengan data yang diberikan

        // Jika linked list kosong, set newNode sebagai head dan tail
        if (head == nullptr) {
            head = tail = newNode;
            currentNode = head; // Set currentNode ke baris pertama
            if (record) {
                // Mencatat aksi INSERT_LINE ke undoStack
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        // Jika menyisipkan di posisi pertama (posisi 0)
        if (position == 0) {
            newNode->next = head;
            head->prev = newNode;
            head = newNode; // Update head ke node baru
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        // Mencari posisi di mana baris baru akan disisipkan
        Node* current = head;
        int index = 0;
        while (current != nullptr && index < position - 1) {
            current = current->next;
            index++;
        }

        // Jika posisi melebihi jumlah baris, sisipkan di akhir
        if (current == tail) {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode; // Update tail ke node baru
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
        else if (current != nullptr) {
            // Menyisipkan di tengah-tengah linked list
            Node* nextNode = current->next;
            newNode->next = nextNode;
            newNode->prev = current;
            current->next = newNode;
            if (nextNode != nullptr) {
                nextNode->prev = newNode;
            }
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
        else {
            // Jika posisi tidak valid, sisipkan di akhir
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
    }

    // Fungsi untuk menghapus baris pada posisi tertentu
    void deleteLine(int position, bool record = true) {
        if (head == nullptr) return; // Tidak ada baris untuk dihapus

        Node* toDelete = head;
        int index = 0;
        while (toDelete != nullptr && index < position) {
            toDelete = toDelete->next;
            index++;
        }

        if (toDelete == nullptr) return; // Posisi tidak valid

        string data = toDelete->data; // Menyimpan data baris yang akan dihapus

        // Menghapus node dari linked list
        if (toDelete == head) {
            head = toDelete->next;
            if (head != nullptr) head->prev = nullptr;
        }
        else if (toDelete == tail) {
            tail = toDelete->prev;
            if (tail != nullptr) tail->next = nullptr;
        }
        else {
            Node* prevNode = toDelete->prev;
            Node* nextNode = toDelete->next;
            if (prevNode != nullptr) prevNode->next = nextNode;
            if (nextNode != nullptr) nextNode->prev = prevNode;
        }

        // Jika node yang dihapus adalah currentNode, perbarui currentNode
        if (toDelete == currentNode) {
            if (toDelete->next != nullptr) {
                currentNode = toDelete->next;
                currentCharIndex = 0;
            }
            else if (toDelete->prev != nullptr) {
                currentNode = toDelete->prev;
                currentCharIndex = currentNode->data.length() - 1;
            }
            else {
                currentNode = nullptr;
                currentCharIndex = 0;
            }
        }

        delete toDelete; // Menghapus node dari memori

        if (record) {
            // Mencatat aksi DELETE_LINE ke undoStack
            undoStack.push(Action(Action::DELETE_LINE, position, data));
            // Kosongkan redoStack karena aksi baru dilakukan
            while (!redoStack.empty()) redoStack.pop();
        }
    }

    // Fungsi untuk menghapus karakter pada posisi saat ini
    void deleteCurrentChar(bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char deletedChar = currentNode->data[currentCharIndex]; // Menyimpan karakter yang akan dihapus
        string before = currentNode->data; // Menyimpan keadaan sebelum perubahan

        // Menghapus karakter dari baris
        currentNode->data.erase(currentCharIndex, 1);
        cout << "Menghapus karakter '" << deletedChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
            if (linePos != -1) {
                // Mencatat aksi DELETE_CHAR ke undoStack
                undoStack.push(Action(Action::DELETE_CHAR, linePos, currentCharIndex, deletedChar, '\0'));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        // Menyesuaikan indeks karakter jika diperlukan setelah penghapusan
        if (currentCharIndex >= currentNode->data.length()) {
            if (currentCharIndex > 0) {
                currentCharIndex--;
            }
        }

        display(); // Menampilkan teks setelah penghapusan
    }

    // Fungsi untuk mengganti karakter pada posisi saat ini
    void replaceCurrentChar(char newChar, bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char oldChar = currentNode->data[currentCharIndex]; // Menyimpan karakter lama
        currentNode->data[currentCharIndex] = newChar;      // Mengganti karakter dengan yang baru
        cout << "Mengganti karakter '" << oldChar << "' dengan '" << newChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
            if (linePos != -1) {
                // Mencatat aksi REPLACE_CHAR ke undoStack
                undoStack.push(Action(Action::REPLACE_CHAR, linePos, currentCharIndex, oldChar, newChar));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        display(); // Menampilkan teks setelah penggantian
    }

    // Fungsi untuk menampilkan seluruh teks dengan highlighting pada currentNode dan currentCharIndex
    void display() {
        Node* current = head;
        while (current != nullptr) {
            // Memeriksa apakah node saat ini adalah currentNode yang di-highlight
            if (current == currentNode) {
                // Jika ada indeks karakter yang di-highlight
                if (currentCharIndex >= 0 && currentCharIndex < current->data.length()) {
                    string highlighted = current->data;
                    // Menambahkan tanda kurung siku di sekitar karakter yang di-highlight
                    highlighted.insert(currentCharIndex + 1, "]");
                    highlighted.insert(currentCharIndex, "[");
                    cout << highlighted;
                }
                else {
                    // Jika tidak ada indeks karakter, highlight seluruh baris
                    cout << "[" << current->data << "]";
                }
            }
            else {
                cout << current->data; // Menampilkan baris tanpa highlight
            }
            current = current->next;
            if (current != nullptr) cout << "\n"; // Menambahkan baris baru jika ada baris berikutnya
        }
        cout << endl;
    }

    // Fungsi untuk menyisipkan baris dan mencatat aksi
    void insertAndTrack(int position, const string& data) {
        insertLine(position, data, true);
    }

    // Fungsi untuk mencari dan menyorot kata kunci dalam teks
    void searchAndHighlight(const string& keyword) {
        Node* current = head;
        bool found = false;
        while (current != nullptr) {
            size_t pos = current->data.find(keyword); // Mencari posisi kata kunci dalam baris
            if (pos != string::npos) {
                found = true;
                string highlighted = current->data;
                // Menambahkan tanda kurung siku di sekitar kata kunci yang ditemukan
                highlighted.insert(pos + keyword.length(), "]");
                highlighted.insert(pos, "[");
                cout << highlighted;
            }
            else {
                cout << current->data; // Menampilkan baris tanpa highlight
            }
            current = current->next;
            if (current != nullptr) cout << "\n"; // Menambahkan baris baru jika ada baris berikutnya
        }
        if (!found) {
            cout << "\nKata kunci \"" << keyword << "\" tidak ditemukan." << endl;
        }
        else {
            cout << endl;
        }
    }

    // Fungsi untuk navigasi ke baris berikutnya
    void moveToNextLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->next != nullptr) {
            currentNode = currentNode->next;    // Pindah ke baris berikutnya
            currentCharIndex = 0;               // Reset indeks karakter ke awal
            cout << "Berpindah ke baris berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di baris terakhir." << endl;
        }
    }

    // Fungsi untuk navigasi ke baris sebelumnya
    void moveToPrevLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->prev != nullptr) {
            currentNode = currentNode->prev;    // Pindah ke baris sebelumnya
            currentCharIndex = 0;               // Reset indeks karakter ke awal
            cout << "Berpindah ke baris sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di baris pertama." << endl;
        }
    }

    // Fungsi untuk navigasi ke karakter berikutnya dalam baris saat ini
    void moveToNextChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < currentNode->data.length() - 1) {
            currentCharIndex++; // Pindah ke karakter berikutnya
            cout << "Berpindah ke karakter berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter terakhir dari baris saat ini." << endl;
        }
    }

    // Fungsi untuk navigasi ke karakter sebelumnya dalam baris saat ini
    void moveToPrevChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex > 0) {
            currentCharIndex--; // Pindah ke karakter sebelumnya
            cout << "Berpindah ke karakter sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter pertama dari baris saat ini." << endl;
        }
    }

    // Fungsi untuk menghapus seluruh baris yang sedang di-highlight
    void deleteCurrentLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris yang dapat dihapus." << endl;
            return;
        }

        int pos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
        if (pos == -1) {
            cout << "Kesalahan dalam mendapatkan posisi baris." << endl;
            return;
        }

        string data = currentNode->data; // Menyimpan data baris yang akan dihapus

        // Mencatat aksi DELETE_LINE ke undoStack
        undoStack.push(Action(Action::DELETE_LINE, pos, data));
        // Kosongkan redoStack karena aksi baru dilakukan
        while (!redoStack.empty()) redoStack.pop();

        // Menghapus currentNode tanpa mencatat ulang aksi
        deleteLine(pos, false);
        cout << "Baris telah dihapus." << endl;

        display(); // Menampilkan teks setelah penghapusan
    }

    // Fungsi untuk melakukan undo terhadap aksi terakhir
    void undo() {
        if (undoStack.empty()) {
            cout << "Tidak ada aksi untuk di-undo." << endl;
            return;
        }

        Action lastAction = undoStack.top(); // Mengambil aksi terakhir dari undoStack
        undoStack.pop();                      // Menghapus aksi dari undoStack

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Undo INSERT_LINE dengan menghapus baris yang di-insert
                deleteLine(lastAction.linePosition, false);
                // Menambahkan aksi ke redoStack untuk memungkinkan redo
                redoStack.push(lastAction);
                cout << "Undo: Menghapus baris yang di-insert." << endl;
                break;

            case Action::DELETE_LINE:
                // Undo DELETE_LINE dengan menyisipkan kembali baris yang dihapus
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Menambahkan aksi ke redoStack untuk memungkinkan redo
                redoStack.push(lastAction);
                cout << "Undo: Menyisipkan kembali baris yang dihapus." << endl;
                break;

            case Action::DELETE_CHAR: {
                // Undo DELETE_CHAR dengan menyisipkan kembali karakter yang dihapus
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex <= targetNode->data.length()) {
                        // Menyisipkan kembali karakter yang dihapus
                        targetNode->data.insert(lastAction.charIndex, string(1, lastAction.oldChar));
                        // Menambahkan aksi ke redoStack
                        redoStack.push(lastAction);
                        cout << "Undo: Menyisipkan kembali karakter yang dihapus." << endl;
                    }
                }
                break;
            }

            case Action::REPLACE_CHAR: {
                // Undo REPLACE_CHAR dengan mengganti kembali karakter ke oldChar
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        // Mengganti karakter kembali ke karakter lama
                        targetNode->data[lastAction.charIndex] = lastAction.oldChar;
                        // Menambahkan aksi ke redoStack
                        redoStack.push(lastAction);
                        cout << "Undo: Mengganti karakter kembali ke '" << lastAction.oldChar << "'." << endl;
                    }
                }
                break;
            }

            default:
                cout << "Aksi tidak dikenali." << endl;
                break;
        }

        display(); // Menampilkan teks setelah undo
    }

    // Fungsi untuk melakukan redo terhadap aksi terakhir yang di-undo
    void redo() {
        if (redoStack.empty()) {
            cout << "Tidak ada aksi untuk di-redo." << endl;
            return;
        }

        Action lastAction = redoStack.top(); // Mengambil aksi terakhir dari redoStack
        redoStack.pop();                      // Menghapus aksi dari redoStack

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Redo INSERT_LINE dengan menyisipkan kembali baris
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Menambahkan aksi ke undoStack untuk memungkinkan undo
                undoStack.push(lastAction);
                cout << "Redo: Menyisipkan kembali baris." << endl;
                break;

            case Action::DELETE_LINE:
                // Redo DELETE_LINE dengan menghapus kembali baris
                deleteLine(lastAction.linePosition, false);
                // Menambahkan aksi ke undoStack untuk memungkinkan undo
                undoStack.push(lastAction);
                cout << "Redo: Menghapus kembali baris." << endl;
                break;

            case Action::DELETE_CHAR: {
                // Redo DELETE_CHAR dengan menghapus kembali karakter
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        char removedChar = targetNode->data[lastAction.charIndex];
                        // Menghapus karakter dari baris
                        targetNode->data.erase(lastAction.charIndex, 1);
                        // Menambahkan aksi ke undoStack
                        undoStack.push(lastAction);
                        cout << "Redo: Menghapus kembali karakter '" << removedChar << "'." << endl;
                    }
                }
                break;
            }

            case Action::REPLACE_CHAR: {
                // Redo REPLACE_CHAR dengan mengganti kembali karakter ke newChar
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        char originalChar = targetNode->data[lastAction.charIndex];
                        // Mengganti karakter ke karakter baru
                        targetNode->data[lastAction.charIndex] = lastAction.newChar;
                        // Menambahkan aksi ke undoStack
                        undoStack.push(lastAction);
                        cout << "Redo: Mengganti karakter kembali ke '" << lastAction.newChar << "'." << endl;
                    }
                }
                break;
            }

            default:
                cout << "Aksi tidak dikenali." << endl;
                break;
        }

        display(); // Menampilkan teks setelah redo
    }
};

int main() {
    LinkedList editor;
    int choice;
    bool running = true;

    // Menambahkan beberapa baris awal ke dalam editor
    editor.insertAndTrack(0, "Hidup itu seperti kopi, ");
    editor.insertAndTrack(1, "terkadang pahit, ");
    editor.insertAndTrack(2, "terkadang manis, ");
    editor.insertAndTrack(3, "tapi selalu bisa dinikmati jika kita tahu caranya.");
    editor.display(); // Menampilkan teks awal

    // Loop utama untuk menampilkan menu dan menerima input pengguna
    while (running) {
        cout << "\n=== Menu Editor Teks ===\n";
        cout << "1. Insert Teks\n";
        cout << "2. Delete Karakter Saat Ini\n";
        cout << "3. Ganti Karakter Saat Ini\n";
        cout << "4. Navigasi ke Baris Berikutnya\n";
        cout << "5. Navigasi ke Baris Sebelumnya\n";
        cout << "6. Navigasi ke Karakter Berikutnya\n";
        cout << "7. Navigasi ke Karakter Sebelumnya\n";
        cout << "8. Cari Kata Kunci\n";
        cout << "9. Undo\n";
        cout << "10. Redo\n";
        cout << "11. Tampilkan Teks\n";
        cout << "12. Hapus Satu Baris\n";
        cout << "13. Keluar\n";
        cout << "Pilih opsi (1-13): ";
        cin >> choice;
        cin.ignore(); // Membersihkan newline dari buffer input

        // Menggunakan switch-case untuk menangani pilihan menu
        switch (choice) {
            case 1: { // Insert Teks
                int position;
                string text;
                cout << "Masukkan posisi untuk menyisipkan teks: ";
                cin >> position;
                cin.ignore(); // Membersihkan newline dari buffer
                cout << "Masukkan teks yang akan disisipkan: ";
                getline(cin, text);
                editor.insertAndTrack(position, text); // Menyisipkan teks pada posisi tertentu
                editor.display(); // Menampilkan teks setelah penyisipan
                break;
            }
            case 2: { // Delete Karakter Saat Ini
                editor.deleteCurrentChar(); // Menghapus karakter yang di-highlight
                break;
            }
            case 3: { // Ganti Karakter Saat Ini
                char newChar;
                cout << "Masukkan karakter baru: ";
                cin >> newChar;
                editor.replaceCurrentChar(newChar); // Mengganti karakter dengan yang baru
                break;
            }
            case 4: { // Navigasi ke Baris Berikutnya
                editor.moveToNextLine(); // Pindah ke baris berikutnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 5: { // Navigasi ke Baris Sebelumnya
                editor.moveToPrevLine(); // Pindah ke baris sebelumnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 6: { // Navigasi ke Karakter Berikutnya
                editor.moveToNextChar(); // Pindah ke karakter berikutnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 7: { // Navigasi ke Karakter Sebelumnya
                editor.moveToPrevChar(); // Pindah ke karakter sebelumnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 8: { // Cari Kata Kunci
                string keyword;
                cout << "Masukkan kata kunci yang akan dicari: ";
                getline(cin, keyword);
                editor.searchAndHighlight(keyword); // Mencari dan menyorot kata kunci
                break;
            }
            case 9: { // Undo
                editor.undo(); // Membatalkan aksi terakhir
                break;
            }
            case 10: { // Redo
                editor.redo(); // Mengulangi aksi yang telah di-undo
                break;
            }
            case 11: { // Tampilkan Teks
                editor.display(); // Menampilkan seluruh teks dengan highlight
                break;
            }
            case 12: { // Hapus Satu Baris
                editor.deleteCurrentLine(); // Menghapus baris yang di-highlight
                break;
            }
            case 13: { // Keluar
                running = false; // Menghentikan loop menu
                cout << "Keluar dari program." << endl;
                break;
            }
            default: { // Penanganan opsi yang tidak valid
                cout << "Opsi tidak valid. Silakan pilih antara 1-13." << endl;
                break;
            }
        }
    }

    return 0;
}#include <iostream>
#include <string>
#include <stack>
using namespace std;

// Struktur untuk menyimpan aksi yang dilakukan oleh pengguna
struct Action {
    // Jenis aksi yang bisa dilakukan
    enum ActionType { 
        INSERT_LINE,    // Menyisipkan baris baru
        DELETE_LINE,    // Menghapus baris
        INSERT_CHAR,    // Menyisipkan karakter
        DELETE_CHAR,    // Menghapus karakter
        REPLACE_CHAR    // Mengganti karakter
    } type;

    int linePosition;    // Posisi baris dalam linked list (dimulai dari 0)
    int charIndex;       // Indeks karakter dalam baris (hanya untuk aksi karakter)
    string data;         // Data yang terkait dengan aksi (untuk INSERT_LINE dan DELETE_LINE)
    char oldChar;        // Karakter lama sebelum diganti (untuk REPLACE_CHAR)
    char newChar;        // Karakter baru setelah diganti (untuk REPLACE_CHAR)

    // Konstruktor untuk aksi INSERT_LINE dan DELETE_LINE
    Action(ActionType type, int linePos, const string& d)
        : type(type), linePosition(linePos), charIndex(-1), data(d), oldChar('\0'), newChar('\0') {}

    // Konstruktor untuk aksi karakter: INSERT_CHAR, DELETE_CHAR, REPLACE_CHAR
    Action(ActionType type, int linePos, int cIndex, char oldC = '\0', char newC = '\0')
        : type(type), linePosition(linePos), charIndex(cIndex), data(""), oldChar(oldC), newChar(newC) {}
};

// Kelas Node merepresentasikan satu baris teks dalam editor
class Node {
public:
    string data; // Teks yang disimpan dalam baris ini
    Node* prev;  // Pointer ke node (baris) sebelumnya
    Node* next;  // Pointer ke node (baris) berikutnya

    // Konstruktor untuk Node
    Node(string data) {
        this->data = data;
        this->prev = nullptr;
        this->next = nullptr;
    }
};

// Kelas LinkedList mengelola daftar baris teks dan operasi terkait
class LinkedList {
private:
    Node* head; // Pointer ke baris pertama dalam linked list
    Node* tail; // Pointer ke baris terakhir dalam linked list

    // Stack untuk menyimpan aksi yang bisa di-undo dan di-redo
    stack<Action> undoStack; // Menyimpan aksi untuk undo
    stack<Action> redoStack; // Menyimpan aksi untuk redo

    // Untuk navigasi dan penyorotan (highlighting)
    Node* currentNode;        // Baris (node) yang sedang di-highlight
    int currentCharIndex;     // Indeks karakter yang sedang di-highlight dalam currentNode

    // Fungsi untuk mendapatkan posisi baris saat ini dalam linked list
    int getCurrentLinePosition() {
        int pos = 0;
        Node* temp = head;
        while (temp != nullptr && temp != currentNode) {
            temp = temp->next;
            pos++;
        }
        return (temp == currentNode) ? pos : -1;
    }

public:
    // Konstruktor untuk LinkedList
    LinkedList() {
        head = nullptr;
        tail = nullptr;
        currentNode = nullptr;
        currentCharIndex = 0;
    }

    // Destruktor untuk membersihkan memori yang dialokasikan
    ~LinkedList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current; // Menghapus node saat ini
            current = next; // Pindah ke node berikutnya
        }
    }

    // Fungsi untuk menyisipkan baris baru pada posisi tertentu
    void insertLine(int position, const string& data, bool record = true) {
        Node* newNode = new Node(data); // Membuat node baru dengan data yang diberikan

        // Jika linked list kosong, set newNode sebagai head dan tail
        if (head == nullptr) {
            head = tail = newNode;
            currentNode = head; // Set currentNode ke baris pertama
            if (record) {
                // Mencatat aksi INSERT_LINE ke undoStack
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        // Jika menyisipkan di posisi pertama (posisi 0)
        if (position == 0) {
            newNode->next = head;
            head->prev = newNode;
            head = newNode; // Update head ke node baru
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        // Mencari posisi di mana baris baru akan disisipkan
        Node* current = head;
        int index = 0;
        while (current != nullptr && index < position - 1) {
            current = current->next;
            index++;
        }

        // Jika posisi melebihi jumlah baris, sisipkan di akhir
        if (current == tail) {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode; // Update tail ke node baru
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
        else if (current != nullptr) {
            // Menyisipkan di tengah-tengah linked list
            Node* nextNode = current->next;
            newNode->next = nextNode;
            newNode->prev = current;
            current->next = newNode;
            if (nextNode != nullptr) {
                nextNode->prev = newNode;
            }
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
        else {
            // Jika posisi tidak valid, sisipkan di akhir
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
    }

    // Fungsi untuk menghapus baris pada posisi tertentu
    void deleteLine(int position, bool record = true) {
        if (head == nullptr) return; // Tidak ada baris untuk dihapus

        Node* toDelete = head;
        int index = 0;
        while (toDelete != nullptr && index < position) {
            toDelete = toDelete->next;
            index++;
        }

        if (toDelete == nullptr) return; // Posisi tidak valid

        string data = toDelete->data; // Menyimpan data baris yang akan dihapus

        // Menghapus node dari linked list
        if (toDelete == head) {
            head = toDelete->next;
            if (head != nullptr) head->prev = nullptr;
        }
        else if (toDelete == tail) {
            tail = toDelete->prev;
            if (tail != nullptr) tail->next = nullptr;
        }
        else {
            Node* prevNode = toDelete->prev;
            Node* nextNode = toDelete->next;
            if (prevNode != nullptr) prevNode->next = nextNode;
            if (nextNode != nullptr) nextNode->prev = prevNode;
        }

        // Jika node yang dihapus adalah currentNode, perbarui currentNode
        if (toDelete == currentNode) {
            if (toDelete->next != nullptr) {
                currentNode = toDelete->next;
                currentCharIndex = 0;
            }
            else if (toDelete->prev != nullptr) {
                currentNode = toDelete->prev;
                currentCharIndex = currentNode->data.length() - 1;
            }
            else {
                currentNode = nullptr;
                currentCharIndex = 0;
            }
        }

        delete toDelete; // Menghapus node dari memori

        if (record) {
            // Mencatat aksi DELETE_LINE ke undoStack
            undoStack.push(Action(Action::DELETE_LINE, position, data));
            // Kosongkan redoStack karena aksi baru dilakukan
            while (!redoStack.empty()) redoStack.pop();
        }
    }

    // Fungsi untuk menghapus karakter pada posisi saat ini
    void deleteCurrentChar(bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char deletedChar = currentNode->data[currentCharIndex]; // Menyimpan karakter yang akan dihapus
        string before = currentNode->data; // Menyimpan keadaan sebelum perubahan

        // Menghapus karakter dari baris
        currentNode->data.erase(currentCharIndex, 1);
        cout << "Menghapus karakter '" << deletedChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
            if (linePos != -1) {
                // Mencatat aksi DELETE_CHAR ke undoStack
                undoStack.push(Action(Action::DELETE_CHAR, linePos, currentCharIndex, deletedChar, '\0'));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        // Menyesuaikan indeks karakter jika diperlukan setelah penghapusan
        if (currentCharIndex >= currentNode->data.length()) {
            if (currentCharIndex > 0) {
                currentCharIndex--;
            }
        }

        display(); // Menampilkan teks setelah penghapusan
    }

    // Fungsi untuk mengganti karakter pada posisi saat ini
    void replaceCurrentChar(char newChar, bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char oldChar = currentNode->data[currentCharIndex]; // Menyimpan karakter lama
        currentNode->data[currentCharIndex] = newChar;      // Mengganti karakter dengan yang baru
        cout << "Mengganti karakter '" << oldChar << "' dengan '" << newChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
            if (linePos != -1) {
                // Mencatat aksi REPLACE_CHAR ke undoStack
                undoStack.push(Action(Action::REPLACE_CHAR, linePos, currentCharIndex, oldChar, newChar));
                // Kosongkan redoStack karena aksi baru dilakukan
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        display(); // Menampilkan teks setelah penggantian
    }

    // Fungsi untuk menampilkan seluruh teks dengan highlighting pada currentNode dan currentCharIndex
    void display() {
        Node* current = head;
        while (current != nullptr) {
            // Memeriksa apakah node saat ini adalah currentNode yang di-highlight
            if (current == currentNode) {
                // Jika ada indeks karakter yang di-highlight
                if (currentCharIndex >= 0 && currentCharIndex < current->data.length()) {
                    string highlighted = current->data;
                    // Menambahkan tanda kurung siku di sekitar karakter yang di-highlight
                    highlighted.insert(currentCharIndex + 1, "]");
                    highlighted.insert(currentCharIndex, "[");
                    cout << highlighted;
                }
                else {
                    // Jika tidak ada indeks karakter, highlight seluruh baris
                    cout << "[" << current->data << "]";
                }
            }
            else {
                cout << current->data; // Menampilkan baris tanpa highlight
            }
            current = current->next;
            if (current != nullptr) cout << "\n"; // Menambahkan baris baru jika ada baris berikutnya
        }
        cout << endl;
    }

    // Fungsi untuk menyisipkan baris dan mencatat aksi
    void insertAndTrack(int position, const string& data) {
        insertLine(position, data, true);
    }

    // Fungsi untuk mencari dan menyorot kata kunci dalam teks
    void searchAndHighlight(const string& keyword) {
        Node* current = head;
        bool found = false;
        while (current != nullptr) {
            size_t pos = current->data.find(keyword); // Mencari posisi kata kunci dalam baris
            if (pos != string::npos) {
                found = true;
                string highlighted = current->data;
                // Menambahkan tanda kurung siku di sekitar kata kunci yang ditemukan
                highlighted.insert(pos + keyword.length(), "]");
                highlighted.insert(pos, "[");
                cout << highlighted;
            }
            else {
                cout << current->data; // Menampilkan baris tanpa highlight
            }
            current = current->next;
            if (current != nullptr) cout << "\n"; // Menambahkan baris baru jika ada baris berikutnya
        }
        if (!found) {
            cout << "\nKata kunci \"" << keyword << "\" tidak ditemukan." << endl;
        }
        else {
            cout << endl;
        }
    }

    // Fungsi untuk navigasi ke baris berikutnya
    void moveToNextLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->next != nullptr) {
            currentNode = currentNode->next;    // Pindah ke baris berikutnya
            currentCharIndex = 0;               // Reset indeks karakter ke awal
            cout << "Berpindah ke baris berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di baris terakhir." << endl;
        }
    }

    // Fungsi untuk navigasi ke baris sebelumnya
    void moveToPrevLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->prev != nullptr) {
            currentNode = currentNode->prev;    // Pindah ke baris sebelumnya
            currentCharIndex = 0;               // Reset indeks karakter ke awal
            cout << "Berpindah ke baris sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di baris pertama." << endl;
        }
    }

    // Fungsi untuk navigasi ke karakter berikutnya dalam baris saat ini
    void moveToNextChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < currentNode->data.length() - 1) {
            currentCharIndex++; // Pindah ke karakter berikutnya
            cout << "Berpindah ke karakter berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter terakhir dari baris saat ini." << endl;
        }
    }

    // Fungsi untuk navigasi ke karakter sebelumnya dalam baris saat ini
    void moveToPrevChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex > 0) {
            currentCharIndex--; // Pindah ke karakter sebelumnya
            cout << "Berpindah ke karakter sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter pertama dari baris saat ini." << endl;
        }
    }

    // Fungsi untuk menghapus seluruh baris yang sedang di-highlight
    void deleteCurrentLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris yang dapat dihapus." << endl;
            return;
        }

        int pos = getCurrentLinePosition(); // Mendapatkan posisi baris saat ini
        if (pos == -1) {
            cout << "Kesalahan dalam mendapatkan posisi baris." << endl;
            return;
        }

        string data = currentNode->data; // Menyimpan data baris yang akan dihapus

        // Mencatat aksi DELETE_LINE ke undoStack
        undoStack.push(Action(Action::DELETE_LINE, pos, data));
        // Kosongkan redoStack karena aksi baru dilakukan
        while (!redoStack.empty()) redoStack.pop();

        // Menghapus currentNode tanpa mencatat ulang aksi
        deleteLine(pos, false);
        cout << "Baris telah dihapus." << endl;

        display(); // Menampilkan teks setelah penghapusan
    }

    // Fungsi untuk melakukan undo terhadap aksi terakhir
    void undo() {
        if (undoStack.empty()) {
            cout << "Tidak ada aksi untuk di-undo." << endl;
            return;
        }

        Action lastAction = undoStack.top(); // Mengambil aksi terakhir dari undoStack
        undoStack.pop();                      // Menghapus aksi dari undoStack

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Undo INSERT_LINE dengan menghapus baris yang di-insert
                deleteLine(lastAction.linePosition, false);
                // Menambahkan aksi ke redoStack untuk memungkinkan redo
                redoStack.push(lastAction);
                cout << "Undo: Menghapus baris yang di-insert." << endl;
                break;

            case Action::DELETE_LINE:
                // Undo DELETE_LINE dengan menyisipkan kembali baris yang dihapus
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Menambahkan aksi ke redoStack untuk memungkinkan redo
                redoStack.push(lastAction);
                cout << "Undo: Menyisipkan kembali baris yang dihapus." << endl;
                break;

            case Action::DELETE_CHAR: {
                // Undo DELETE_CHAR dengan menyisipkan kembali karakter yang dihapus
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex <= targetNode->data.length()) {
                        // Menyisipkan kembali karakter yang dihapus
                        targetNode->data.insert(lastAction.charIndex, string(1, lastAction.oldChar));
                        // Menambahkan aksi ke redoStack
                        redoStack.push(lastAction);
                        cout << "Undo: Menyisipkan kembali karakter yang dihapus." << endl;
                    }
                }
                break;
            }

            case Action::REPLACE_CHAR: {
                // Undo REPLACE_CHAR dengan mengganti kembali karakter ke oldChar
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        // Mengganti karakter kembali ke karakter lama
                        targetNode->data[lastAction.charIndex] = lastAction.oldChar;
                        // Menambahkan aksi ke redoStack
                        redoStack.push(lastAction);
                        cout << "Undo: Mengganti karakter kembali ke '" << lastAction.oldChar << "'." << endl;
                    }
                }
                break;
            }

            default:
                cout << "Aksi tidak dikenali." << endl;
                break;
        }

        display(); // Menampilkan teks setelah undo
    }

    // Fungsi untuk melakukan redo terhadap aksi terakhir yang di-undo
    void redo() {
        if (redoStack.empty()) {
            cout << "Tidak ada aksi untuk di-redo." << endl;
            return;
        }

        Action lastAction = redoStack.top(); // Mengambil aksi terakhir dari redoStack
        redoStack.pop();                      // Menghapus aksi dari redoStack

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Redo INSERT_LINE dengan menyisipkan kembali baris
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Menambahkan aksi ke undoStack untuk memungkinkan undo
                undoStack.push(lastAction);
                cout << "Redo: Menyisipkan kembali baris." << endl;
                break;

            case Action::DELETE_LINE:
                // Redo DELETE_LINE dengan menghapus kembali baris
                deleteLine(lastAction.linePosition, false);
                // Menambahkan aksi ke undoStack untuk memungkinkan undo
                undoStack.push(lastAction);
                cout << "Redo: Menghapus kembali baris." << endl;
                break;

            case Action::DELETE_CHAR: {
                // Redo DELETE_CHAR dengan menghapus kembali karakter
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        char removedChar = targetNode->data[lastAction.charIndex];
                        // Menghapus karakter dari baris
                        targetNode->data.erase(lastAction.charIndex, 1);
                        // Menambahkan aksi ke undoStack
                        undoStack.push(lastAction);
                        cout << "Redo: Menghapus kembali karakter '" << removedChar << "'." << endl;
                    }
                }
                break;
            }

            case Action::REPLACE_CHAR: {
                // Redo REPLACE_CHAR dengan mengganti kembali karakter ke newChar
                Node* targetNode = head;
                int index = 0;
                while (targetNode != nullptr && index < lastAction.linePosition) {
                    targetNode = targetNode->next;
                    index++;
                }
                if (targetNode != nullptr) {
                    if (lastAction.charIndex >= 0 && lastAction.charIndex < targetNode->data.length()) {
                        char originalChar = targetNode->data[lastAction.charIndex];
                        // Mengganti karakter ke karakter baru
                        targetNode->data[lastAction.charIndex] = lastAction.newChar;
                        // Menambahkan aksi ke undoStack
                        undoStack.push(lastAction);
                        cout << "Redo: Mengganti karakter kembali ke '" << lastAction.newChar << "'." << endl;
                    }
                }
                break;
            }

            default:
                cout << "Aksi tidak dikenali." << endl;
                break;
        }

        display(); // Menampilkan teks setelah redo
    }
};

int main() {
    LinkedList editor;
    int choice;
    bool running = true;

    // Menambahkan beberapa baris awal ke dalam editor
    editor.insertAndTrack(0, "Hidup itu seperti kopi, ");
    editor.insertAndTrack(1, "terkadang pahit, ");
    editor.insertAndTrack(2, "terkadang manis, ");
    editor.insertAndTrack(3, "tapi selalu bisa dinikmati jika kita tahu caranya.");
    editor.display(); // Menampilkan teks awal

    // Loop utama untuk menampilkan menu dan menerima input pengguna
    while (running) {
        cout << "\n=== Menu Editor Teks ===\n";
        cout << "1. Insert Teks\n";
        cout << "2. Delete Karakter Saat Ini\n";
        cout << "3. Ganti Karakter Saat Ini\n";
        cout << "4. Navigasi ke Baris Berikutnya\n";
        cout << "5. Navigasi ke Baris Sebelumnya\n";
        cout << "6. Navigasi ke Karakter Berikutnya\n";
        cout << "7. Navigasi ke Karakter Sebelumnya\n";
        cout << "8. Cari Kata Kunci\n";
        cout << "9. Undo\n";
        cout << "10. Redo\n";
        cout << "11. Tampilkan Teks\n";
        cout << "12. Hapus Satu Baris\n";
        cout << "13. Keluar\n";
        cout << "Pilih opsi (1-13): ";
        cin >> choice;
        cin.ignore(); // Membersihkan newline dari buffer input

        // Menggunakan switch-case untuk menangani pilihan menu
        switch (choice) {
            case 1: { // Insert Teks
                int position;
                string text;
                cout << "Masukkan posisi untuk menyisipkan teks: ";
                cin >> position;
                cin.ignore(); // Membersihkan newline dari buffer
                cout << "Masukkan teks yang akan disisipkan: ";
                getline(cin, text);
                editor.insertAndTrack(position, text); // Menyisipkan teks pada posisi tertentu
                editor.display(); // Menampilkan teks setelah penyisipan
                break;
            }
            case 2: { // Delete Karakter Saat Ini
                editor.deleteCurrentChar(); // Menghapus karakter yang di-highlight
                break;
            }
            case 3: { // Ganti Karakter Saat Ini
                char newChar;
                cout << "Masukkan karakter baru: ";
                cin >> newChar;
                editor.replaceCurrentChar(newChar); // Mengganti karakter dengan yang baru
                break;
            }
            case 4: { // Navigasi ke Baris Berikutnya
                editor.moveToNextLine(); // Pindah ke baris berikutnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 5: { // Navigasi ke Baris Sebelumnya
                editor.moveToPrevLine(); // Pindah ke baris sebelumnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 6: { // Navigasi ke Karakter Berikutnya
                editor.moveToNextChar(); // Pindah ke karakter berikutnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 7: { // Navigasi ke Karakter Sebelumnya
                editor.moveToPrevChar(); // Pindah ke karakter sebelumnya
                editor.display();        // Menampilkan teks setelah navigasi
                break;
            }
            case 8: { // Cari Kata Kunci
                string keyword;
                cout << "Masukkan kata kunci yang akan dicari: ";
                getline(cin, keyword);
                editor.searchAndHighlight(keyword); // Mencari dan menyorot kata kunci
                break;
            }
            case 9: { // Undo
                editor.undo(); // Membatalkan aksi terakhir
                break;
            }
            case 10: { // Redo
                editor.redo(); // Mengulangi aksi yang telah di-undo
                break;
            }
            case 11: { // Tampilkan Teks
                editor.display(); // Menampilkan seluruh teks dengan highlight
                break;
            }
            case 12: { // Hapus Satu Baris
                editor.deleteCurrentLine(); // Menghapus baris yang di-highlight
                break;
            }
            case 13: { // Keluar
                running = false; // Menghentikan loop menu
                cout << "Keluar dari program." << endl;
                break;
            }
            default: { // Penanganan opsi yang tidak valid
                cout << "Opsi tidak valid. Silakan pilih antara 1-13." << endl;
                break;
            }
        }
    }

    return 0;
}