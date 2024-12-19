#include <iostream>
#include <string>
#include <stack>
using namespace std;

// Struktur untuk menyimpan aksi
struct Action {
    enum ActionType { INSERT_LINE, DELETE_LINE, INSERT_CHAR, DELETE_CHAR, REPLACE_CHAR } type;
    int linePosition;    // Posisi baris dalam linked list
    int charIndex;       // Indeks karakter dalam baris (hanya untuk aksi karakter)
    string data;         // Data yang terkait (untuk INSERT_LINE dan DELETE_LINE)
    char oldChar;        // Karakter lama (untuk REPLACE_CHAR)
    char newChar;        // Karakter baru (untuk REPLACE_CHAR)

    // Konstruktor untuk INSERT_LINE dan DELETE_LINE
    Action(ActionType type, int linePos, const string& d)
        : type(type), linePosition(linePos), charIndex(-1), data(d), oldChar('\0'), newChar('\0') {}

    // Konstruktor untuk INSERT_CHAR, DELETE_CHAR, dan REPLACE_CHAR
    Action(ActionType type, int linePos, int cIndex, char oldC = '\0', char newC = '\0')
        : type(type), linePosition(linePos), charIndex(cIndex), data(""), oldChar(oldC), newChar(newC) {}
};

class Node {
public:
    string data; // Data yang disimpan (baris teks)
    Node* prev;  // Pointer ke node sebelumnya
    Node* next;  // Pointer ke node berikutnya

    Node(string data) {
        this->data = data;
        this->prev = nullptr;
        this->next = nullptr;
    }
};

class LinkedList {
private:
    Node* head; // Awal linked list
    Node* tail; // Akhir linked list

    // Stack untuk undo dan redo
    stack<Action> undoStack; // Menyimpan aksi untuk undo
    stack<Action> redoStack; // Menyimpan aksi untuk redo

    // Untuk navigasi dan highlighting
    Node* currentNode;        // Node (baris) saat ini
    int currentCharIndex;     // Indeks karakter saat ini dalam currentNode

    // Mendapatkan posisi currentNode
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
    LinkedList() {
        head = nullptr;
        tail = nullptr;
        currentNode = nullptr;
        currentCharIndex = 0;
    }

    ~LinkedList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }

    // Menyisipkan node pada posisi tertentu
    void insertLine(int position, const string& data, bool record = true) {
        Node* newNode = new Node(data);

        if (head == nullptr) {
            head = tail = newNode;
            currentNode = head;
            if (record) {
                // Aksi INSERT_LINE pada posisi 0
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                // Kosongkan redoStack
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        if (position == 0) {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, 0, data));
                while (!redoStack.empty()) redoStack.pop();
            }
            return;
        }

        Node* current = head;
        int index = 0;
        while (current != nullptr && index < position - 1) {
            current = current->next;
            index++;
        }

        if (current == tail) {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
        else if (current != nullptr) {
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
            // Posisi melebihi panjang list, sisipkan di akhir
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
            if (record) {
                undoStack.push(Action(Action::INSERT_LINE, index + 1, data));
                while (!redoStack.empty()) redoStack.pop();
            }
        }
    }

    // Menghapus node pada posisi tertentu
    void deleteLine(int position, bool record = true) {
        if (head == nullptr) return;

        Node* toDelete = head;
        int index = 0;
        while (toDelete != nullptr && index < position) {
            toDelete = toDelete->next;
            index++;
        }

        if (toDelete == nullptr) return; // Posisi tidak valid

        string data = toDelete->data;

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

        delete toDelete;

        if (record) {
            undoStack.push(Action(Action::DELETE_LINE, position, data));
            while (!redoStack.empty()) redoStack.pop();
        }
    }

    // Menghapus karakter pada posisi saat ini
    void deleteCurrentChar(bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char deletedChar = currentNode->data[currentCharIndex];
        string before = currentNode->data;

        // Menghapus karakter
        currentNode->data.erase(currentCharIndex, 1);
        cout << "Menghapus karakter '" << deletedChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition();
            if (linePos != -1) {
                // Simpan aksi DELETE_CHAR
                undoStack.push(Action(Action::DELETE_CHAR, linePos, currentCharIndex, deletedChar, '\0'));
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        // Jika setelah penghapusan, indeks berada di luar batas, sesuaikan
        if (currentCharIndex >= currentNode->data.length()) {
            if (currentCharIndex > 0) {
                currentCharIndex--;
            }
        }

        display();
    }

    // Mengganti karakter pada posisi saat ini
    void replaceCurrentChar(char newChar, bool record = true) {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < 0 || currentCharIndex >= currentNode->data.length()) {
            cout << "Indeks karakter saat ini di luar batas." << endl;
            return;
        }

        char oldChar = currentNode->data[currentCharIndex];
        currentNode->data[currentCharIndex] = newChar;
        cout << "Mengganti karakter '" << oldChar << "' dengan '" << newChar << "' pada posisi " << currentCharIndex << "." << endl;

        if (record) {
            int linePos = getCurrentLinePosition();
            if (linePos != -1) {
                // Simpan aksi REPLACE_CHAR
                undoStack.push(Action(Action::REPLACE_CHAR, linePos, currentCharIndex, oldChar, newChar));
                while (!redoStack.empty()) redoStack.pop();
            }
        }

        display();
    }

    // Menampilkan seluruh teks dengan highlighting
    void display() {
        Node* current = head;
        while (current != nullptr) {
            // Cek apakah current node adalah currentNode
            if (current == currentNode) {
                // Jika highlight karakter tertentu
                if (currentCharIndex >= 0 && currentCharIndex < current->data.length()) {
                    string highlighted = current->data;
                    highlighted.insert(currentCharIndex + 1, "]");
                    highlighted.insert(currentCharIndex, "[");
                    cout << highlighted;
                }
                else {
                    // Jika indeks karakter tidak valid, highlight seluruh baris
                    cout << "[" << current->data << "]";
                }
            }
            else {
                cout << current->data;
            }
            current = current->next;
            if (current != nullptr) cout << "\n";
        }
        cout << endl;
    }

    // Menambahkan baris dan merekam aksi
    void insertAndTrack(int position, const string& data) {
        insertLine(position, data, true);
    }

    // Mencari dan menyorot kata kunci
    void searchAndHighlight(const string& keyword) {
        Node* current = head;
        bool found = false;
        while (current != nullptr) {
            size_t pos = current->data.find(keyword);
            if (pos != string::npos) {
                found = true;
                string highlighted = current->data;
                highlighted.insert(pos + keyword.length(), "]");
                highlighted.insert(pos, "[");
                cout << highlighted;
            }
            else {
                cout << current->data;
            }
            current = current->next;
            if (current != nullptr) cout << "\n";
        }
        if (!found) {
            cout << "\nKata kunci \"" << keyword << "\" tidak ditemukan." << endl;
        }
        else {
            cout << endl;
        }
    }

    // Navigasi ke baris berikutnya
    void moveToNextLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->next != nullptr) {
            currentNode = currentNode->next;
            currentCharIndex = 0; // Reset karakter ke awal
            cout << "Berpindah ke baris berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di baris terakhir." << endl;
        }
    }

    // Navigasi ke baris sebelumnya
    void moveToPrevLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris untuk dinavigasi." << endl;
            return;
        }

        if (currentNode->prev != nullptr) {
            currentNode = currentNode->prev;
            currentCharIndex = 0; // Reset karakter ke awal
            cout << "Berpindah ke baris sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di baris pertama." << endl;
        }
    }

    // Navigasi ke karakter berikutnya
    void moveToNextChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex < currentNode->data.length() - 1) {
            currentCharIndex++;
            cout << "Berpindah ke karakter berikutnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter terakhir dari baris saat ini." << endl;
        }
    }

    // Navigasi ke karakter sebelumnya
    void moveToPrevChar() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris tersedia." << endl;
            return;
        }

        if (currentCharIndex > 0) {
            currentCharIndex--;
            cout << "Berpindah ke karakter sebelumnya." << endl;
        }
        else {
            cout << "Sudah berada di karakter pertama dari baris saat ini." << endl;
        }
    }

    // Menghapus seluruh baris yang sedang di-highlight
    void deleteCurrentLine() {
        if (currentNode == nullptr) {
            cout << "Tidak ada baris yang dapat dihapus." << endl;
            return;
        }

        int pos = getCurrentLinePosition();
        if (pos == -1) {
            cout << "Kesalahan dalam mendapatkan posisi baris." << endl;
            return;
        }

        string data = currentNode->data;

        // Simpan aksi DELETE_LINE ke undoStack
        undoStack.push(Action(Action::DELETE_LINE, pos, data));
        while (!redoStack.empty()) redoStack.pop();

        // Hapus currentNode
        deleteLine(pos, false);
        cout << "Baris telah dihapus." << endl;

        display();
    }

    // Undo aksi terakhir
    void undo() {
        if (undoStack.empty()) {
            cout << "Tidak ada aksi untuk di-undo." << endl;
            return;
        }

        Action lastAction = undoStack.top();
        undoStack.pop();

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Undo INSERT_LINE dengan menghapus baris yang di-insert
                deleteLine(lastAction.linePosition, false);
                // Simpan aksi ke redoStack
                redoStack.push(lastAction);
                cout << "Undo: Menghapus baris yang di-insert." << endl;
                break;

            case Action::DELETE_LINE:
                // Undo DELETE_LINE dengan menyisipkan kembali baris yang dihapus
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Simpan aksi ke redoStack
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
                        targetNode->data.insert(lastAction.charIndex, string(1, lastAction.oldChar));
                        // Simpan aksi ke redoStack
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
                        targetNode->data[lastAction.charIndex] = lastAction.oldChar;
                        // Simpan aksi ke redoStack
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

        display();
    }

    // Redo aksi terakhir
    void redo() {
        if (redoStack.empty()) {
            cout << "Tidak ada aksi untuk di-redo." << endl;
            return;
        }

        Action lastAction = redoStack.top();
        redoStack.pop();

        switch (lastAction.type) {
            case Action::INSERT_LINE:
                // Redo INSERT_LINE dengan menyisipkan kembali baris
                insertLine(lastAction.linePosition, lastAction.data, false);
                // Simpan aksi ke undoStack
                undoStack.push(lastAction);
                cout << "Redo: Menyisipkan kembali baris." << endl;
                break;

            case Action::DELETE_LINE:
                // Redo DELETE_LINE dengan menghapus kembali baris
                deleteLine(lastAction.linePosition, false);
                // Simpan aksi ke undoStack
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
                        targetNode->data.erase(lastAction.charIndex, 1);
                        // Simpan aksi ke undoStack
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
                        targetNode->data[lastAction.charIndex] = lastAction.newChar;
                        // Simpan aksi ke undoStack
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

        display();
    }
};

int main() {
    LinkedList editor;
    int choice;
    bool running = true;

    // Menambahkan beberapa baris awal
    editor.insertAndTrack(0, "Hidup itu seperti kopi, ");
    editor.insertAndTrack(1, "terkadang pahit, ");
    editor.insertAndTrack(2, "terkadang manis, ");
    editor.insertAndTrack(3, "tapi selalu bisa dinikmati jika kita tahu caranya.");
    editor.display();

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
        cin.ignore(); // Membersihkan newline dari buffer

        switch (choice) {
            case 1: {
                int position;
                string text;
                cout << "Masukkan posisi untuk menyisipkan teks: ";
                cin >> position;
                cin.ignore(); // Membersihkan newline dari buffer
                cout << "Masukkan teks yang akan disisipkan: ";
                getline(cin, text);
                editor.insertAndTrack(position, text);
                editor.display();
                break;
            }
            case 2: {
                editor.deleteCurrentChar();
                break;
            }
            case 3: {
                char newChar;
                cout << "Masukkan karakter baru: ";
                cin >> newChar;
                editor.replaceCurrentChar(newChar);
                break;
            }
            case 4: {
                editor.moveToNextLine();
                editor.display();
                break;
            }
            case 5: {
                editor.moveToPrevLine();
                editor.display();
                break;
            }
            case 6: {
                editor.moveToNextChar();
                editor.display();
                break;
            }
            case 7: {
                editor.moveToPrevChar();
                editor.display();
                break;
            }
            case 8: {
                string keyword;
                cout << "Masukkan kata kunci yang akan dicari: ";
                getline(cin, keyword);
                editor.searchAndHighlight(keyword);
                break;
            }
            case 9: {
                editor.undo();
                break;
            }
            case 10: {
                editor.redo();
                break;
            }
            case 11: {
                editor.display();
                break;
            }
            case 12: { // Opsi baru untuk menghapus satu baris
                editor.deleteCurrentLine();
                break;
            }
            case 13: { // Penyesuaian nomor opsi untuk keluar
                running = false;
                cout << "Keluar dari program." << endl;
                break;
            }
            default: {
                cout << "Opsi tidak valid. Silakan pilih antara 1-13." << endl;
                break;
            }
        }
    }

    return 0;
}
