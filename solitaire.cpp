#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <set>

#include <ctime>
#include <random>
#include <algorithm>

#define INITIAL_WIDTH 1200
#define INITIAL_HEIGHT 950

#define DEFAULT_SUIT_LENGTH 13
#define DEFAULT_NO_OF_SUITS 4
#define DEFAULT_NO_OF_TABLEAUS 7

// Master(ful) SCREEN Enum 
enum class Screen {
    Home,
    Game,
    Settings
};

SDL_Renderer* gRenderer = nullptr;
SDL_Window* gWindow = nullptr;

bool quit {false};
bool game_is_running {false};
bool has_changed {true};
Screen screen = Screen::Home;

class Stack;

bool dragged {false};
Stack* gStack = nullptr;

bool aiMode {false}; // 19 Oct 2024 - this is going to be bad

int scrWidth = INITIAL_WIDTH;
int scrHeight = INITIAL_HEIGHT;

enum class Suit { Hearts, Diamonds, Clubs, Spades };

enum class GameStatus {
    Good,
    TextureLoadError,
    DeckInitError,
    LogicError
};

enum class ChangeListener {
	Stock,
	Waste,
	Tableau,
	Foundation,
	Nothing
};

std::vector<ChangeListener> changeListener;
std::vector<int> changeIDXs; // removed the int[2] implementation of index track and things
void addToChangeListener(ChangeListener pile_type, int IDX) { changeListener.push_back(pile_type); changeIDXs.push_back(IDX); }
void clearChangeListener() { changeListener.clear(); changeIDXs.clear(); }

GameStatus gStatus = GameStatus::Good;

namespace SDLW {
    struct SuitRankHash {
        std::size_t operator()(const std::pair<Suit, int>& pair) const {
            return std::hash<int>()(static_cast<int>(pair.first)) ^ std::hash<int>()(pair.second);
        }
    };

    const std::unordered_map<std::pair<Suit, int>, std::string, SuitRankHash> cardPaths = {
        // Hearts
        {{Suit::Hearts, 1}, "assets/cards/red/Hearts_A.png"},
        {{Suit::Hearts, 2}, "assets/cards/red/Hearts_2.png"},
        {{Suit::Hearts, 3}, "assets/cards/red/Hearts_3.png"},
        {{Suit::Hearts, 4}, "assets/cards/red/Hearts_4.png"},
        {{Suit::Hearts, 5}, "assets/cards/red/Hearts_5.png"},
        {{Suit::Hearts, 6}, "assets/cards/red/Hearts_6.png"},
        {{Suit::Hearts, 7}, "assets/cards/red/Hearts_7.png"},
        {{Suit::Hearts, 8}, "assets/cards/red/Hearts_8.png"},
        {{Suit::Hearts, 9}, "assets/cards/red/Hearts_9.png"},
        {{Suit::Hearts, 10}, "assets/cards/red/Hearts_10.png"},
        {{Suit::Hearts, 11}, "assets/cards/red/Hearts_Jack.png"},
        {{Suit::Hearts, 12}, "assets/cards/red/Hearts_Queen.png"},
        {{Suit::Hearts, 13}, "assets/cards/red/Hearts_King.png"},
        // Diamonds
        {{Suit::Diamonds, 1}, "assets/cards/red/Tiles_A.png"},
        {{Suit::Diamonds, 2}, "assets/cards/red/Tiles_2.png"},
        {{Suit::Diamonds, 3}, "assets/cards/red/Tiles_3.png"},
        {{Suit::Diamonds, 4}, "assets/cards/red/Tiles_4.png"},
        {{Suit::Diamonds, 5}, "assets/cards/red/Tiles_5.png"},
        {{Suit::Diamonds, 6}, "assets/cards/red/Tiles_6.png"},
        {{Suit::Diamonds, 7}, "assets/cards/red/Tiles_7.png"},
        {{Suit::Diamonds, 8}, "assets/cards/red/Tiles_8.png"},
        {{Suit::Diamonds, 9}, "assets/cards/red/Tiles_9.png"},
        {{Suit::Diamonds, 10}, "assets/cards/red/Tiles_10.png"},
        {{Suit::Diamonds, 11}, "assets/cards/red/Tiles_Jack.png"},
        {{Suit::Diamonds, 12}, "assets/cards/red/Tiles_Queen.png"},
        {{Suit::Diamonds, 13}, "assets/cards/red/Tiles_King.png"},
        // Clubs
        {{Suit::Clubs, 1}, "assets/cards/black/Clovers_A.png"},
        {{Suit::Clubs, 2}, "assets/cards/black/Clovers_2.png"},
        {{Suit::Clubs, 3}, "assets/cards/black/Clovers_3.png"},
        {{Suit::Clubs, 4}, "assets/cards/black/Clovers_4.png"},
        {{Suit::Clubs, 5}, "assets/cards/black/Clovers_5.png"},
        {{Suit::Clubs, 6}, "assets/cards/black/Clovers_6.png"},
        {{Suit::Clubs, 7}, "assets/cards/black/Clovers_7.png"},
        {{Suit::Clubs, 8}, "assets/cards/black/Clovers_8.png"},
        {{Suit::Clubs, 9}, "assets/cards/black/Clovers_9.png"},
        {{Suit::Clubs, 10}, "assets/cards/black/Clovers_10.png"},
        {{Suit::Clubs, 11}, "assets/cards/black/Clovers_Jack.png"},
        {{Suit::Clubs, 12}, "assets/cards/black/Clovers_Queen.png"},
        {{Suit::Clubs, 13}, "assets/cards/black/Clovers_King.png"},
        // Spades
        {{Suit::Spades, 1}, "assets/cards/black/Pikes_A.png"},
        {{Suit::Spades, 2}, "assets/cards/black/Pikes_2.png"},
        {{Suit::Spades, 3}, "assets/cards/black/Pikes_3.png"},
        {{Suit::Spades, 4}, "assets/cards/black/Pikes_4.png"},
        {{Suit::Spades, 5}, "assets/cards/black/Pikes_5.png"},
        {{Suit::Spades, 6}, "assets/cards/black/Pikes_6.png"},
        {{Suit::Spades, 7}, "assets/cards/black/Pikes_7.png"},
        {{Suit::Spades, 8}, "assets/cards/black/Pikes_8.png"},
        {{Suit::Spades, 9}, "assets/cards/black/Pikes_9.png"},
        {{Suit::Spades, 10}, "assets/cards/black/Pikes_10.png"},
        {{Suit::Spades, 11}, "assets/cards/black/Pikes_Jack.png"},
        {{Suit::Spades, 12}, "assets/cards/black/Pikes_Queen.png"},
        {{Suit::Spades, 13}, "assets/cards/black/Pikes_King.png"}
    };

    const std::string& getCardPath(Suit suit, int rank) {
        static const std::string emptyString = "";
        auto it = cardPaths.find({suit, rank});
        return (it != cardPaths.end()) ? it->second : emptyString;
    }

    SDL_Texture* loadTexture(std::string path) {
        SDL_Surface* loadedSurface = IMG_Load(path.c_str());
        if (!loadedSurface) {
            std::cerr << "Unable to load image: " << path << ", error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        SDL_Texture* newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        if (!newTexture) {
            std::cerr << "Unable to create texture from surface, path: " << path << ", error: " << SDL_GetError() << std::endl;
        }
        SDL_FreeSurface(loadedSurface);
        return newTexture;
    }

    void setTarget(SDL_Texture* texture) {
        SDL_SetRenderTarget(gRenderer, texture);
    }

    void setWindowTarget() {
        setTarget(nullptr);
    }

    void drawTextureAbsolute(SDL_Texture* texture, const SDL_Rect& rect) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(gRenderer, texture, nullptr, &rect);
    }

    void drawToTexture(SDL_Texture* target, SDL_Texture* source, const SDL_Rect& srcRect, const SDL_Rect& destRect) { // this for compatibility reasons
        setTarget(target);
        SDL_RenderCopy(gRenderer, source, &srcRect, &destRect);
        setTarget(nullptr);
    }

    void drawToTexture(SDL_Texture* texture, const SDL_Rect& rect) { // basically the same thing (oof)
        drawTextureAbsolute(texture, rect);
    }

    void drawToWindow(SDL_Texture* texture, const SDL_Rect& rect) {
        drawTextureAbsolute(texture, rect);
    }

    const std::string backTexturePath = "assets/cards/back.png";

    void renderClear() {
        SDL_RenderClear(gRenderer);
    }

    void renderPresent() {
        SDL_RenderPresent(gRenderer);
    }

    bool mouseInRect(const SDL_Rect& rect, const SDL_Point& point) {
        return SDL_PointInRect(&point, &rect);
    }
}

enum class Alignment {
    Vertical,
    Horizontal
};

enum class Colour {
    Red,
    Black
};

class Card {
private:
    Suit suit;
    int rank;
    bool visible;
    SDL_Texture* texture;
    SDL_Rect rect;

public:
	static const int Ace = 1; // RANKS ARE 1-INDEXED (1 to 13)
	static const int King = 13;

    Card() : suit(Suit::Hearts), rank(1), visible(false), texture(nullptr), rect{0, 0, 0, 0} {}

    int getRank() const { return rank; }
    Suit getSuit() const { return suit; }
    Colour getColour() const {
        return (suit == Suit::Hearts || suit == Suit::Diamonds) ? Colour::Red : Colour::Black;
    }
    bool isVisible() const { return visible; }
    SDL_Texture* getTexture() const { return texture; }
    const SDL_Rect& getRect() const { return rect; }
    int getCardX() const { return rect.x; }
    int getCardY() const { return rect.y; }

    void setRank(int new_rank) { rank = new_rank; }
    void setSuit(Suit new_suit) { suit = new_suit; }
    void setVisible(bool new_visible) { visible = new_visible; }
    void setTexture(SDL_Texture* new_texture) { texture = new_texture; }
    void setRect(const SDL_Rect& new_rect) { rect = new_rect; }
};

class Pile {
private:
    std::vector<Card*> cards;
    Alignment alignment;
    int offset;
    int x, y;
    int cardWidth, cardHeight;
    SDL_Texture* pileTexture;
    SDL_Rect pileRect;

public:
    static SDL_Texture* cardBackTexture;

    Pile() : pileTexture(nullptr), pileRect{0, 0, 0, 0} {}

    ~Pile() {
        if (pileTexture) {
            SDL_DestroyTexture(pileTexture);
        }
    }

    void setAlignment(Alignment new_alignment) { alignment = new_alignment; }
    void setOffset(int new_offset) { offset = new_offset; }
    void setPosition(int new_x, int new_y) { x = new_x; y = new_y; }

    void updatePileTexture() {
        if (pileTexture) {
            SDL_DestroyTexture(pileTexture);
            pileTexture = nullptr;
        }
        pileTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cardWidth, getPileTextureHeight());
        if (!pileTexture) {
            std::cerr << "Error initialising pileTexture for pile" << std::endl;
        }
    }

    void setCardDimensions(int new_width, int new_height) { 
        cardWidth = new_width; 
        cardHeight = new_height;
        updatePileTexture();
        // std::cout << "Card Dimensions set: " << cardWidth << "x" << cardHeight << std::endl; // Log dimensions
    }

    void setPileRect() {
        pileRect.x = x;
        pileRect.y = y;
        pileRect.h = isTableau() ? getPileTextureHeight() : cardHeight;
        pileRect.w = cardWidth;
    }

    void addCard(Card* card) {
        cards.push_back(card);
    }

    Card* top() const {
        return !cards.empty() ? cards.back() : nullptr;
    }

    void removeCard() {
        if (top()) cards.pop_back();
    }

    void renderAllCards() {
        SDLW::setTarget(pileTexture);
        SDLW::renderClear();

        SDL_Rect rect;
        rect.w = cardWidth;
        rect.h = cardHeight;
        rect.x = 0; // x is locally 0 regardless

        if (isTableau()) {
            for (size_t i = 0; i < size(); ++i) {
                Card* card = cards[i];
                rect.y = i * offset; // Stack vertically with the defined offset

                card->setRect({getX(), getY() + rect.y, cardWidth, cardHeight});

                if (card->isVisible()) {
                    SDLW::drawToTexture(card->getTexture(), rect);
                } else {
                    SDLW::drawToTexture(cardBackTexture, rect);
                }
            }
        } else {
            if (!cards.empty()) {
                Card* topCard = top();
                rect.y = 0; // Always start at 0 for non-tableau

                topCard->setRect({getX(), getY(), cardWidth, cardHeight});

                if (topCard->isVisible()) {
                    SDLW::drawToTexture(topCard->getTexture(), rect);
                } else {
                    SDLW::drawToTexture(cardBackTexture, rect);
                }
            }
        }
    }

    void clearAllCards() { cards.clear(); }
    auto begin() { return cards.begin(); }
    auto end() { return cards.end(); }
    auto begin() const { return cards.cbegin(); }
    auto end() const { return cards.cend(); }

    int getCardWidth() const { return cardWidth; }
    int getCardHeight() const { return cardHeight; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getOffset() const { return offset; }
    bool isTableau() const { return alignment == Alignment::Vertical; }
    bool empty() const { return cards.empty(); }
    size_t size() const { return cards.size(); }

    const Card* operator[](int idx) const {
    	if (idx >= 0 && idx < size()) return cards[idx];
		std::cerr<<"Operator[] access in Pile is out of range"<<std::endl; return nullptr;
    }
    Card* operator[](int idx) {
    	if (idx >= 0 && idx < size()) return cards[idx];
		std::cerr<<"Operator[] access in Pile is out of range"<<std::endl; return nullptr;
    }

    int getPileTextureHeight() const { return (offset * (size() - 1)) + cardHeight; } // for non zero values
    SDL_Texture* getPileTexture() const { return pileTexture; }
    const SDL_Rect& getPileRect() const { return pileRect; }

    void recomputeAndRender() {
    	setPileRect(); // forgot to change the rects HERE actually, the other is just texture
    	updatePileTexture(); // forgot to change rects
    	renderAllCards();
    	// this entire function was initially renderPileAgain()
    }

    void setTopVisible() { if (!empty()) cards.back()->setVisible(true); }
    void setTopInvisible() { if (!empty()) cards.back()->setVisible(false); }
};
SDL_Texture* Pile::cardBackTexture = nullptr;

namespace DeckFormulae {
    int getGlobalCardW() {
        return scrWidth / 10;
    }

    int getGlobalCardH() {
        return static_cast<int>(scrHeight / 6.5);
    }

    int getTableauOffset() {
        return getGlobalCardH() / 4;
    }

    int getTableauX(int idx) {
        return (scrWidth / 7) * idx + getGlobalCardW() / 4;
    }

    int getTableauY(int) {
        return scrHeight / 4;
    }

    int getFoundationX(int idx) {
        return scrWidth - (4 - idx) * (getGlobalCardW() + 20);
    }

    int getFoundationY(int) {
        return scrHeight / 15;
    }

    int getStockX() {
        return scrWidth / 20;
    }

    int getStockY() {
        return scrHeight / 15;
    }

    int getWasteX() {
        return getStockX() + getGlobalCardW() + 20;
    }

    int getWasteY() {
        return getStockY();
    }
}

class Deck {
private:
    std::vector<Pile> tableaus;
    std::vector<Pile> foundations;
    Pile stock;
    Pile waste;

    std::vector<Card> cardStore;

    void initCards() {
        for (int i = 0; i < no_of_suits; ++i) {
            for (int j = 0; j < suit_length; ++j) {
                Card card;
                card.setSuit(static_cast<Suit>(i));
                card.setRank(j + 1);
                card.setVisible(false);

                SDL_Texture* texture = SDLW::loadTexture(SDLW::getCardPath(card.getSuit(), card.getRank()));
                if (!texture) {
                    std::cerr << "Error loading texture for suit " << static_cast<int>(card.getSuit()) 
                              << " and rank " << card.getRank() << std::endl;
                    gStatus = GameStatus::TextureLoadError;
                    return;
                }
                card.setTexture(texture);
                cardStore.push_back(card);
            }
        }
    }

    void initStock() {
        stock.setAlignment(Alignment::Horizontal);
        for (const Card& card : cardStore) {
            stock.addCard(const_cast<Card*>(&card)); // Ensure we use the correct reference
        }
    }

    void initWaste() {
    	waste.setAlignment(Alignment::Horizontal);
    }

    void shuffleStock() {
        std::shuffle(stock.begin(), stock.end(), std::default_random_engine(static_cast<unsigned int>(std::time(0))));
    }

    void transferToTableaus() {
        for (int i = 0; i < no_of_tableaus; ++i) {
            tableaus[i].setAlignment(Alignment::Vertical);
            for (int j = 0; j <= i; ++j) {
                Card* card_ptr = stock.top();
                stock.removeCard();
                tableaus[i].addCard(const_cast<Card*>(card_ptr));
                if (j == i) {
                    const_cast<Card*>(card_ptr)->setVisible(true);
                }
            }
        }
    }

    void initFoundations() {
        for (int i = 0; i < no_of_suits; ++i) {
            foundations[i].setAlignment(Alignment::Horizontal);
        }
    }

    /* void makeRemainderStockVisible() {
    	for (int i=0; i<stock.size(); i++) {
    		stock[i]->setVisible(true);
    	}
    } */

    void initPiles() {
        initStock();
        initWaste();
        initFoundations();
        shuffleStock();
        transferToTableaus();
        // makeRemainderStockVisible();
    }

    bool setCardBackTexture() {
        Pile::cardBackTexture = SDLW::loadTexture(SDLW::backTexturePath);
        if (!Pile::cardBackTexture) {
            std::cerr << "Error in loading card back texture: " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    }

    void destroyCardBackTexture() {
        SDL_DestroyTexture(Pile::cardBackTexture);
        Pile::cardBackTexture = nullptr;
    }

    void destroyAllTextures() {
        for (Card& card : cardStore) {
            SDL_DestroyTexture(card.getTexture());
        }
        destroyCardBackTexture();
    }

    void manageStockWasteDimensions() {
        stock.setPosition(DeckFormulae::getStockX(), DeckFormulae::getStockY());
        stock.setOffset(1); // TENTATIVE
        stock.setCardDimensions(DeckFormulae::getGlobalCardW(), DeckFormulae::getGlobalCardH());
        stock.setPileRect();

        waste.setPosition(DeckFormulae::getWasteX(), DeckFormulae::getWasteY());
        waste.setOffset(1); // TENTATIVE
        waste.setCardDimensions(DeckFormulae::getGlobalCardW(), DeckFormulae::getGlobalCardH());
        waste.setPileRect();
    }

    void manageFoundationDimensions() {
        for (int i = 0; i < no_of_suits; ++i) {
            foundations[i].setPosition(DeckFormulae::getFoundationX(i), DeckFormulae::getFoundationY(i));
            foundations[i].setOffset(1); // TENTATIVE
            foundations[i].setCardDimensions(DeckFormulae::getGlobalCardW(), DeckFormulae::getGlobalCardH());
            foundations[i].setPileRect();
        }
    }

    void manageTableauDimensions() {
        for (int i = 0; i < no_of_tableaus; ++i) {
            tableaus[i].setPosition(DeckFormulae::getTableauX(i), DeckFormulae::getTableauY(i));
            tableaus[i].setOffset(DeckFormulae::getTableauOffset());
            tableaus[i].setCardDimensions(DeckFormulae::getGlobalCardW(), DeckFormulae::getGlobalCardH());
            tableaus[i].setPileRect();
        }
    }

    void manageDimensions() {
        manageStockWasteDimensions();
        manageFoundationDimensions();
        manageTableauDimensions();
    }

public:
	const int suit_length;
    const int no_of_suits;
    const int no_of_tableaus;

    Deck(int suit_length, int no_of_suits, int no_of_tableaus)
        : suit_length(suit_length), no_of_suits(no_of_suits), no_of_tableaus(no_of_tableaus),
          tableaus(no_of_tableaus), foundations(no_of_suits) {
        if (!setCardBackTexture()) {
            std::cerr<<"Could not set card back texture in Deck constructor"<<std::endl;
        }
        initCards();
        initPiles();
        manageDimensions();
    }

    ~Deck() {
        destroyAllTextures();
    }

    void renderAllPiles() {
        for (auto& tableau : tableaus) {
            tableau.renderAllCards();
        }
        for (auto& foundation : foundations) {
            foundation.renderAllCards();
        }
        stock.renderAllCards();
        waste.renderAllCards();
        std::cout<<"Does it renderAllPiles()?"<<std::endl;
    }

    void drawAllPiles() {
        for (auto& tableau : tableaus) {
            SDLW::drawToWindow(tableau.getPileTexture(), tableau.getPileRect());
        }
        for (auto& foundation : foundations) {
            SDLW::drawToWindow(foundation.getPileTexture(), foundation.getPileRect());
        }
        SDLW::drawToWindow(stock.getPileTexture(), stock.getPileRect());
        SDLW::drawToWindow(waste.getPileTexture(), waste.getPileRect());
    }

    void renderPileAgain(Pile& pile) { // this should be used instead
    	pile.recomputeAndRender();
    }

    void redrawPile(Pile& pile) { // this is really not used anywhere
        pile.renderAllCards();
        drawAllPiles();
    }

    void onResize() {
        manageDimensions();
        renderAllPiles();
    }

    Pile& getTableau(int index) { return tableaus.at(index); }
	Pile& getFoundation(int index) { return foundations.at(index); }
	Pile& getStock() { return stock; }
	Pile& getWaste() { return waste; }
};

// GDECK DECLARATION HERE
Deck* gDeck = nullptr;

namespace Statistics { // a forward declaration for messy change handling in Stack when tableau's top is removed on transfer(ance)
	void setChangeMadeInCycle();
}

class Stack {
private:
	Pile& originPile;
	std::vector<Card*> cards;

	SDL_Texture* stackTexture;
	int x, y;

public:
	Stack(Pile& originPile) : originPile(originPile), stackTexture(nullptr), x(originPile.getX()), y(originPile.getY()) {}
	~Stack() {
		if (!empty()) returnStackToOriginPile();
		if (stackTexture) SDL_DestroyTexture(stackTexture);
	}

	int getX() const { return x; }
	int getY() const { return y; }
	size_t size() const { return cards.size(); }
	bool empty() const { return cards.empty(); }
	int getStackTextureHeight() const { return (originPile.getOffset() * (size() - 1)) + originPile.getCardHeight(); }
	SDL_Rect getStackRect() const { return {x, y, originPile.getCardWidth(), getStackTextureHeight()}; }
	SDL_Texture* getStackTexture() const { return stackTexture; }

	Card* top() { return empty() ? nullptr : cards.back(); }
	Card* bottom() { return empty() ? nullptr : cards.front(); }

	void setStackPosition(int new_x, int new_y) { x = new_x; y = new_y; }

	Card* operator[](int idx) {
		if (idx >= 0 && idx < size()) return cards[idx];
		std::cerr<<"Operator[] access in Card-Stack is out of range"<<std::endl; return nullptr;
	}

	void updateStackTexture() {
    	if (stackTexture) {
    		SDL_DestroyTexture(stackTexture); stackTexture = nullptr;
    	}
    	stackTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, originPile.getCardWidth(), getStackTextureHeight());
    	if (!stackTexture) {
    		std::cerr<<"Error creating stack texture from "<<((originPile.isTableau())?"tableau":"non-tableau pile")<<std::endl;
    	}
    }

    void addCardToStack(Card* card) { cards.push_back(card); }
	void fillStackFromCardIndexInOrigin(int idx) {
	    if (idx >= 0 && idx < originPile.size()) {
	        // Add cards to the stack from the given index to the end
	        for (size_t i = idx; i < originPile.size(); i++) {
	            addCardToStack(originPile[i]);
	        }

	        // Remove cards from originPile starting from the end to the index
	        for (size_t i = originPile.size(); i > idx; i--) {
	            originPile.removeCard(); // This will remove the last card each time
	        }
	    } else {
	        std::cerr << "Invalid idx (" << idx << ") provided in fillStackFromCardIndexInOrigin()" << std::endl;
	    }
	}
	void fillStackWithTopOfOrigin() { // usable only with waste and foundation mousedowns
		if (originPile.top()) {
			Card* card = originPile.top();
			originPile.removeCard();
			addCardToStack(card);
		}
	}
    void returnStackToOriginPile() {
    	for (size_t i=0; i<size(); i++) {
    		originPile.addCard(cards[i]);
    	}
    	cards.clear();
    }
    void handleOriginPileVisibility() {
		if (originPile.isTableau() && !originPile.empty()) {
			if (!originPile.top()->isVisible()) {
				Statistics::setChangeMadeInCycle();
				originPile.top()->setVisible(true);
			}
		}
	}
    void transferStackToNewPile(Pile& newPile) {
    	for (size_t i=0; i<size(); i++) {
    		newPile.addCard(cards[i]);
    	}
    	cards.clear();
    	handleOriginPileVisibility();
    }

    void updateStackCardsRects() { // use only this when dragging stack around
    	int actualX = getX();
    	const int offset = originPile.getOffset();

    	for (size_t i=0; i<size(); i++) {
    		int actualY = i * offset;

    		cards[i]->setRect({actualX, actualY, originPile.getCardWidth(), originPile.getCardHeight()});
    	}
    }
    void renderStack() { // DOES NOT RECREATE STACK TEXTURE: updates card rects and stack
    	SDLW::setTarget(stackTexture);
    	SDLW::renderClear();

    	SDL_Rect rect;

    	rect.x = 0;
    	rect.w = originPile.getCardWidth();
    	rect.h = originPile.getCardHeight();

		int actualX = getX();

    	for (size_t i=0; i<size(); i++) {
    		Card* card = cards[i];
    		rect.y = i * originPile.getOffset();

    		int actualY = getY() + rect.y;
    		card->setRect({actualX, actualY, originPile.getCardWidth(), originPile.getCardHeight()});

    		SDLW::drawToTexture(card->getTexture(), rect);
    		// stacks can only contain visible cards, would check if visible when creating stack
    	}
    }
};

namespace Statistics {
	int total_score {0};

	const int score_CardRevealedOnTableau {2};
	const int score_CardDrawnFromStock {2};
	const int score_CardPutOnFoundation {5};
	const int score_CycleThroughStock {-2};
	const int score_CardDrawnFromFoundation {-5};

	int getScore() { return total_score; }
	void resetScore() { total_score = 0; }

	void addToScore(int num) { total_score += num; }

	void onCardRevealedOnTableau() { addToScore(score_CardRevealedOnTableau); }
	void onCardDrawnFromStock() { addToScore(score_CardDrawnFromStock); }
	void onCardPutonFoundation() { addToScore(score_CardPutOnFoundation); }
	void onCycleThroughStock() { addToScore(score_CycleThroughStock); }
	void onCardDrawnFromFoundation() { addToScore(score_CardDrawnFromFoundation); }

	void printScore() { std::cout<<"--- Current score is "<<total_score<<std::endl; }

	// bools for targeting change
	bool change_was_made_in_cycle = false;
	// and their methods
	bool getChangeMadeInCycle() { return change_was_made_in_cycle; }
	void setChangeMadeInCycle() { change_was_made_in_cycle = true; }
	void clearChangeMadeInCycle() { change_was_made_in_cycle = false; }

	// game win/lose condition handling
	bool checkGameLoseCondition() { return !getChangeMadeInCycle(); }
	bool onCycleComplete() {
		if (checkGameLoseCondition()) {
			std::cout<<"You did not make any valid moves this time around and have reached end of cycle."<<std::endl; // no cards unveiled or drawn from stock/waste
			std::cout<<"Continue game? Y or N"<<std::endl;
			char tmp;
			std::cin>>tmp;
			if (tmp == 'Y') {
				// draw all from waste back to stock on bool check
			return true;
			} else {
				// placeholder: terminate game with a fxn and return
				return false;
			}
		} else {
			clearChangeMadeInCycle();
			return true;
		}
	}
}

/*
GAME WIN CONDITION NOT IMPLEMENTED YET (placeholder)
*/

namespace Logic {
    // memory check card
    bool memcheckCard(Card* card_ptr, const std::string& str) {
        if (!card_ptr) {
            std::cerr << "Invalid card pointer detected in Logic functions" << std::endl;
            std::cerr<<str<<std::endl;
            return false;
        }
        return true;
    }

    // boolean checks for card movements
    bool canMoveCardFromTableau(Card* card_ptr, Pile& tableau) {
        if (!memcheckCard(card_ptr, "from canMoveCardFromTableau()")) return false;
        return card_ptr->isVisible();
    }
    bool foundation_CanStackCardOnCard(Card* upper_card, Card* bottom_card) {
    	if (memcheckCard(upper_card, "first - from foundation_canStackCardOnCard() -- rudimentary")) {
    		if (!bottom_card) {
    			return upper_card->getRank() == Card::Ace;
    		} else {
    			return upper_card->getSuit() == bottom_card->getSuit() && upper_card->getRank() == (bottom_card->getRank() + 1);
    		}
    	}
    	return false;
    }
    bool tableau_CanStackCardOnCard(Card* upper_card, Card* bottom_card) {
    	if (memcheckCard(upper_card, "first - from tableau_canStackCardOnCard() -- rudimentary")) {
    		if (!bottom_card) {
    			return upper_card->getRank() == Card::King;
    		} else {
    			return upper_card->getColour() != bottom_card->getColour() && upper_card->getRank() == (bottom_card->getRank() - 1);
    		}
    	}
    	return false;
    }
    bool canMoveCardToTableau(Card* card_ptr, Pile& tableau) {
        return tableau_CanStackCardOnCard(card_ptr, tableau.top());
    }
    bool canMoveStackToTableau(Stack& stack, Pile& tableau) {
        return canMoveCardToTableau(stack.bottom(), tableau);
    }
    bool canMoveCardToFoundation(Card* card_ptr, Pile& foundation) {
    	return foundation_CanStackCardOnCard(card_ptr, foundation.top());
    }
    bool canMoveStackToFoundation(Stack& stack, Pile& foundation) {
    	return (stack.size() == 1) ? canMoveCardToFoundation(stack.bottom(), foundation) : false;
    }

    // getters for mouse events
    int getMaxTableauY(Deck& deck) {
        int max_cards = 0; 
        size_t idx = 0;  // Initialize index
        for (size_t i = 0; i < deck.no_of_tableaus; i++) {
            Pile& tableau = deck.getTableau(i);
            if (tableau.size() > max_cards) {
                max_cards = tableau.size();
                idx = i;  // Store index of the tableau with the maximum cards
            }
        }
        return deck.getTableau(idx).getY() + deck.getTableau(idx).getPileTextureHeight();  // Correct usage of idx
    }

    // boolean checks for mouse events
    bool mouseOnCard(SDL_Point& mouse_point, Card* card_ptr) {
        if (!card_ptr) {
            std::cerr << "Invalid/null card_ptr passed to mouseOnCard()" << std::endl;
            return false;
        }
        return SDLW::mouseInRect(card_ptr->getRect(), mouse_point);
    }
    bool mouseOnPile(SDL_Point& mouse_point, Pile& pile) {
        return SDLW::mouseInRect(pile.getPileRect(), mouse_point);
    }
    bool mouseOnStock(SDL_Point& mouse_point, Deck& deck) {
        return mouseOnPile(mouse_point, deck.getStock());
    }

    bool mouseOnWaste(SDL_Point& mouse_point, Deck& deck) {
        return mouseOnPile(mouse_point, deck.getWaste());
    }
    bool mouseOnTableauSpace(SDL_Point& mouse_point, Deck& deck) {
        if (mouse_point.y >= deck.getTableau(0).getY() && mouse_point.y <= getMaxTableauY(deck)) {
            Pile& last_tableau = deck.getTableau(deck.no_of_tableaus - 1);
            if (mouse_point.x >= deck.getTableau(0).getX() && mouse_point.x <= last_tableau.getX() + last_tableau.getCardWidth()) {
                return true;
            }
        }
        return false; 
    }
    bool mouseOnFoundationSpace(SDL_Point& mouse_point, Deck& deck) { // Includes stock and waste
        if (mouse_point.y >= deck.getStock().getY() && mouse_point.y <= deck.getStock().getY() + deck.getStock().getCardHeight()) {
            Pile& last_foundation = deck.getFoundation(deck.no_of_suits - 1);
            if (mouse_point.x >= deck.getStock().getX() && mouse_point.x <= last_foundation.getX() + last_foundation.getCardWidth()) {
                return true;
            }
        }
        return false;
    }
    // size_t checks for mouse events
    int mouseOnWhichFoundation(SDL_Point& mouse_point, Deck& deck) {
        for (size_t i = 0; i < deck.no_of_suits; i++) {
            if (mouseOnPile(mouse_point, deck.getFoundation(i))) {
                return static_cast<int>(i);  // Return index of the foundation if mouse is over it
            }
        }
        return -1;  // Return -1 if no foundation is under the mouse
    }
	int mouseOnWhichTableau(SDL_Point& mouse_point, Deck& deck) {
	    for (size_t i = 0; i < deck.no_of_tableaus; i++) {
	        if (mouseOnPile(mouse_point, deck.getTableau(i))) {
	            return static_cast<int>(i);
	        }
	    }
	    return -1;  // Return -1 if no tableau is under the mouse
	}
	/* int mouseOnWhichCardInTableauOld(SDL_Point& mouse_point, Pile& tableau) { // outdated function, does not position offset correctly
	    for (size_t i = 0; i < tableau.size(); i++) {
	        if (tableau[i]->isVisible() && mouseOnCard(mouse_point, tableau[i])) {
	            return static_cast<int>(i); // Return index of the visible card
	        }
	    }
	    return -1; // No card found
	} */
	int mouseOnWhichCardInTableau(SDL_Point& mouse_point, Pile& tableau) {
		for (size_t i = tableau.size()-1; i >= 0 && tableau[i]->isVisible(); i--) {
			if (mouseOnCard(mouse_point, tableau[i])) {
				return static_cast<int>(i);
			}
		}
		return -1;
	}
}

namespace Meta { // forward declaration: forward declared Meta functions for Operations
	void loadCachedTexture();
	void destroyCachedTexture();
}

void quitGame(); // forward declaration for game quit operation handles

namespace Operations {

	ChangeListener pile_type_drawn_from = ChangeListener::Nothing;
	// handle pile drawn checking
	void clearPileTypeDrawnFrom() { pile_type_drawn_from = ChangeListener::Nothing; }
	void setPileTypeDrawnFrom(ChangeListener new_pile_drawn_from) { pile_type_drawn_from = new_pile_drawn_from; }
	ChangeListener getPileTypeDrawnFrom() { return pile_type_drawn_from; }

	// foundation registry for checking if cards were stored
	std::set<Card*> foundationRegistry;
	bool wasInFoundationRegistry(Card* card_ptr) {
		return foundationRegistry.find(card_ptr) != foundationRegistry.end();
	}
	void addToFoundationRegistry(Card* card_ptr) { foundationRegistry.insert(card_ptr); }
	void clearFoundationRegistry() { foundationRegistry.clear(); }

	// animation placeholder:
	void animPlaceholder() {}

	// victory placeholder:
	void victoryPoint() {}

	// offset mechanisms:
	SDL_Point drag_offset = {0, 0};
	void setDragOffsetToZero() { drag_offset = {0, 0}; }
	void computeDragOffset(SDL_Point& mp) { drag_offset = {mp.x - gStack->getX(), mp.y - gStack->getY()}; }

	// card movements:
	void setStWaVis(Pile& stock, Pile& waste) { // set stock and waste visibilities
		stock.setTopInvisible(); waste.setTopVisible();
	}
	bool transferAllFromWasteToStock(Pile& stock, Pile& waste) { // set to bool to not perform normal game operations anymore on game quit
		if (Statistics::onCycleComplete()) {
			while (!waste.empty()) {
				stock.addCard(waste.top());
				waste.removeCard();
			}
			std::cout<<"Transferred all from waste to stock"<<std::endl;
			return true;
		} else {
			std::cout<<"Quittin' game or something"<<std::endl;
			quitGame();
			return false;
		}
	}
	void moveFromStockToWaste(Pile& stock, Pile& waste) {
		if (stock.empty()) {
			if (!transferAllFromWasteToStock(stock, waste)) return;
			setStWaVis(stock, waste);
			animPlaceholder();
			// changeListener.push_back(ChangeListener::Stock); // changeListener handles rendering+drawing
			// changeListener.push_back(ChangeListener::Waste);
			addToChangeListener(ChangeListener::Stock, 0);
			addToChangeListener(ChangeListener::Waste, 0);
			has_changed = true;
		} else {
			Card* card = stock.top();
			stock.removeCard();
			waste.addCard(card);
			setStWaVis(stock, waste);
			animPlaceholder();

			addToChangeListener(ChangeListener::Stock, 0);
			addToChangeListener(ChangeListener::Waste, 0);
			has_changed = true;
		}
	}

	// drag and stack functions:
	void setDragged(SDL_Point& mp) { computeDragOffset(mp); dragged = true; has_changed = true; }
	void clearDragged() { setDragOffsetToZero(); dragged = false; has_changed = true; }
	void destroyStack() {
		if (gStack) delete gStack;
		gStack = nullptr;
		Meta::destroyCachedTexture();
	}
	void actuallyRenderStack() {
		gStack->updateStackTexture();
		gStack->renderStack(); // render stack only on stack creation
	}
	void createStack(Pile& pile) { // still hacky but hopefully less so
		if (gStack) destroyStack();
		gStack = new Stack(pile);
		if (!gStack) std::cerr<<"Stack creation failure"<<std::endl;
	}
	// if ChangeListener.size() ever stops being >2 we're doomed (or <0 too); well as long as it's Klondike it shouldn't...
	void prepareCachedTexture(Pile& pile) { // groups together redundant stack creation preparations
		pile.recomputeAndRender(); // render the pile the stack is created from before loading cached texture; fixed: use recomputeAndRender() than renderAllCards()
		actuallyRenderStack();
		Meta::loadCachedTexture();
	}
	void createStackFromWaste(Pile& waste) { // THIS FOR WASTE
		createStack(waste);
		gStack->fillStackWithTopOfOrigin();
		addToChangeListener(ChangeListener::Waste, 0); // implemented some ChangeListener hacks with [0] and [1]s; 19 Oct 2024 - Not anymore! >:)
		prepareCachedTexture(waste); // render waste (origin) now
	}
	void createStackFromTableau(Pile& tableau, int tableau_idx, int idx) { // THIS FOR TABLEAU
		createStack(tableau);
		gStack->fillStackFromCardIndexInOrigin(idx);
		gStack->setStackPosition(gStack->bottom()->getCardX(), gStack->bottom()->getCardY());
		addToChangeListener(ChangeListener::Tableau, tableau_idx);
		prepareCachedTexture(tableau);
	}
	void createStackFromFoundation(Pile& foundation, int foundation_idx) {
		createStack(foundation);
		gStack->fillStackWithTopOfOrigin();
		addToChangeListener(ChangeListener::Foundation, foundation_idx);
		prepareCachedTexture(foundation);
	}

	// down handlers:
	void handleDownOnStock(SDL_Point& mp, Deck& deck) {
		std::cout<<"Handle down on stock reached"<<std::endl;
		moveFromStockToWaste(deck.getStock(), deck.getWaste());
	}
	void handleDownOnWaste(SDL_Point& mp, Deck& deck) {
		// selects single card to drag from waste
		std::cout<<"Handle down on waste reached"<<std::endl;
		if (!deck.getWaste().empty()) {
			createStackFromWaste(deck.getWaste());
			setDragged(mp);
			animPlaceholder();

			setPileTypeDrawnFrom(ChangeListener::Waste);
		}
	}
	void handleDownOnFoundation(SDL_Point& mp, int foundation_idx, Deck& deck) {
		// selects a card from foundation
		std::cout<<"Handle down on foundation "<<foundation_idx<<" reached"<<std::endl;
		Pile& foundation = deck.getFoundation(foundation_idx);

		if (!foundation.empty()) {
			createStackFromFoundation(foundation, foundation_idx);
			setDragged(mp);
			animPlaceholder();

			setPileTypeDrawnFrom(ChangeListener::Foundation); // set pile type is drawn from a foundation
		} 
	}
	void handleDownOnTableau(SDL_Point& mp, int tableau_idx, Deck& deck) {
		// placeholder
		std::cout<<"Handle down on tableau "<<tableau_idx<<" reached"<<std::endl;
		Pile& tableau = deck.getTableau(tableau_idx);

		if (!tableau.empty()) {
			int idx = Logic::mouseOnWhichCardInTableau(mp, tableau);
			if (idx != -1 && tableau[idx]->isVisible()) {
				createStackFromTableau(tableau, tableau_idx, idx);
				setDragged(mp);
				animPlaceholder();

				setPileTypeDrawnFrom(ChangeListener::Tableau); // set pile type is drawn from a tableau
			}
		}
	}

	// up handlers:
	void handleUpDefault() {
		if (!gStack) {
			std::cerr<<"gStack null for handleUpDefault() call"<<std::endl;
		} else {
			clearPileTypeDrawnFrom();
			destroyStack();
			clearDragged();
		}
	}
	void handleUpOnFoundation(int idx, Deck& deck) {
		// sees if stack is loadable to the clicked foundation
		std::cout<<"Handle up on foundation "<<idx<<" reached"<<std::endl;

		Pile& foundation = deck.getFoundation(idx);

		if (Logic::canMoveStackToFoundation(*gStack, foundation)) {
			gStack->transferStackToNewPile(foundation);
			addToChangeListener(ChangeListener::Foundation, idx);

			// does not matter which card is cleared, if a card was taking space for another card in stock, then game can continue
			// but since card can be drawn back from the foundation, this should not decide thaty
			// clearPileTypeDrawnFrom();
			// Statistics::setChangeMadeInCycle();

			// writing a replacement now
			if (!wasInFoundationRegistry(foundation.top())) {
				addToFoundationRegistry(foundation.top());
				Statistics::setChangeMadeInCycle();
			}
		}
		handleUpDefault();
	}
	void handleUpOnTableau(int idx, Deck& deck) {
		// placeholder
		std::cout<<"Handle up on tableau "<<idx<<" reached"<<std::endl;

		Pile& tableau = deck.getTableau(idx);

		if (Logic::canMoveStackToTableau(*gStack, tableau)) {
			gStack->transferStackToNewPile(tableau);
			addToChangeListener(ChangeListener::Tableau, idx);

			if (getPileTypeDrawnFrom() == ChangeListener::Waste) { // if card drawn from stock/waste to tableau, change is true and game can continue
				Statistics::setChangeMadeInCycle();
			}
		}
		handleUpDefault();
	}

	// ensembles:
	bool mouseDownHandled(SDL_Event& e, Deck& deck) { // fxns are bool so if needed can be if-ed
		SDL_Point mp = {e.button.x, e.button.y};
		if (Logic::mouseOnFoundationSpace(mp, deck)) {
			if (Logic::mouseOnStock(mp, deck)) {
				handleDownOnStock(mp, deck);
				return true;
			} else if (Logic::mouseOnWaste(mp, deck)) {
				handleDownOnWaste(mp, deck);
				return true;
			} else {
				int idx = Logic::mouseOnWhichFoundation(mp, deck);
				if (idx != -1) {
					handleDownOnFoundation(mp, idx, deck);
					return true;
				}
				return false;
			}
		} else if (Logic::mouseOnTableauSpace(mp, deck)) {
			int idx = Logic::mouseOnWhichTableau(mp, deck);
			if (idx != -1) {
				handleDownOnTableau(mp, idx, deck);
				return true;
			}
			return false;
		}
		return false;
	}
	bool mouseMotionHandled(SDL_Event& e, Deck& deck) {
		// updates stack's Card* rects but does not actually re-render stack's batch texture, just draws it again
		if (dragged) {
			gStack->setStackPosition(e.motion.x - drag_offset.x, e.motion.y - drag_offset.y);
			gStack->updateStackCardsRects();
			std::cout<<"Mouse motion is being handled"<<std::endl;
			has_changed = true; // so that GameLoop draws stack and cachedTexture
			return true;
		}
		return false;
	}
	bool mouseUpHandled(SDL_Event& e, Deck& deck) {
		// placeholder
		if (dragged) { // only needed for when dragged
			if (!gStack) {
				std::cerr<<"gStack does not even exist, what's being dragged? mouseUpHandler(), more importantly how did it get here?"<<std::endl;
				return false;	
			}

			SDL_Point mp = {e.button.x, e.button.y};
			if (Logic::mouseOnFoundationSpace(mp, deck)) {
				int idx = Logic::mouseOnWhichFoundation(mp, deck);
				if (idx != -1) {
					handleUpOnFoundation(idx, deck);
					return true;
				}
				handleUpDefault();
				return true;
			} else if (Logic::mouseOnTableauSpace(mp, deck)) {
				int idx = Logic::mouseOnWhichTableau(mp, deck);
				if (idx != -1) {
					handleUpOnTableau(idx, deck);
					return true;
				}
				handleUpDefault();
				return true;
			}
			handleUpDefault();
			return true;
		}
		return false;
	}
}

namespace Meta {
	SDL_Texture* titleTexture = nullptr;
	SDL_Texture* backgroundTexture = nullptr;
	SDL_Texture* homeTexture = nullptr;
	SDL_Texture* playTexture = nullptr;
	SDL_Texture* settingsTexture = nullptr;
	SDL_Texture* quitTexture = nullptr;
	SDL_Texture* returnTexture = nullptr;

	SDL_Rect titleRect = {0, 0, 0, 0};
	SDL_Rect backgroundRect = {0, 0, 0, 0};
	SDL_Rect homeRect = {0, 0, 0, 0};
	SDL_Rect playRect = {0, 0, 0, 0};
	SDL_Rect settingsRect = {0, 0, 0, 0};
	SDL_Rect quitRect = {0, 0, 0, 0};
	SDL_Rect returnRect = {0, 0, 0, 0};

	SDL_Rect gHomeRect = {0, 0, 0, 0};
    SDL_Rect gSettingsRect = {0, 0, 0, 0};

	int buttonWidth = 458;
	int buttonHeight = 227;

	int titleWidth = 557;
	int titleHeight = 543;

	int gameButtonWidth = 200;
	int gameButtonHeight = 150;

	bool initTextures() {
	    titleTexture = SDLW::loadTexture("assets/title.png");
	    if (!titleTexture) {
	        std::cerr << "Title texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    backgroundTexture = SDLW::loadTexture("assets/woodsplash.png");
	    if (!backgroundTexture) {
	        std::cerr << "Background texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    homeTexture = SDLW::loadTexture("assets/home.png");
	    if (!homeTexture) {
	        std::cerr << "Home texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    playTexture = SDLW::loadTexture("assets/play.png");
	    if (!playTexture) {
	        std::cerr << "Play texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    settingsTexture = SDLW::loadTexture("assets/settings.png");
	    if (!settingsTexture) {
	        std::cerr << "Settings texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    quitTexture = SDLW::loadTexture("assets/quit.png");
	    if (!quitTexture) {
	        std::cerr << "Quit texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }
	    returnTexture = SDLW::loadTexture("assets/return.png");
	    if (!returnTexture) {
	        std::cerr << "Return texture load failure: " << SDL_GetError() << std::endl;
	        return false;
	    }

	    return true;
	}
	void closeTextures() {
        if (titleTexture) {
            SDL_DestroyTexture(titleTexture);
            titleTexture = nullptr;
        }
        if (backgroundTexture) {
            SDL_DestroyTexture(backgroundTexture);
            backgroundTexture = nullptr;
        }
        if (homeTexture) {
            SDL_DestroyTexture(homeTexture);
            homeTexture = nullptr;
        }
        if (playTexture) {
            SDL_DestroyTexture(playTexture);
            playTexture = nullptr;
        }
        if (settingsTexture) {
            SDL_DestroyTexture(settingsTexture);
            settingsTexture = nullptr;
        }
        if (quitTexture) {
            SDL_DestroyTexture(quitTexture);
            quitTexture = nullptr;
        }
        if (returnTexture) {
            SDL_DestroyTexture(returnTexture);
            returnTexture = nullptr;
        }
    }

	namespace mform {
	    int getTitleWidth() { return scrWidth * 0.8; }
	    int getTitleHeight() { return scrHeight * 0.26; }
	    int getTitleX() { return (scrWidth - getTitleWidth()) / 2; }
	    int getTitleY() { return scrHeight * 0.1; }
	    int getButtonWidth() { return scrWidth * 0.3; }
	    int getButtonHeight() { return scrHeight * 0.1; }
	    int getPlayX() { return (scrWidth - getButtonWidth()) / 2; }
	    int getPlayY() { return scrHeight - getButtonHeight() - (0.5*scrHeight); }
	    int getSettingsX() { return (scrWidth - getButtonWidth()) / 2; }
	    int getSettingsY() { return scrHeight - getButtonHeight() - (0.2*scrHeight); }
	    int getHomeX() { return getSettingsX(); }
	    int getHomeY() { return getSettingsY(); }
	    int getReturnX() { return getPlayX(); }
	    int getReturnY() { return getPlayY(); }
	    // Game Screen Specific Formulas
	    int getGameButtonWidth() { return scrWidth * 0.2; } // Adjusted width
	    int getGameButtonHeight() { return scrHeight * 0.07; } // Adjusted height

		int getGameHomeX() { return scrWidth - getGameButtonWidth() - (scrWidth * 0.01); } // 1% from the right edge
		int getGameHomeY() { return scrHeight * 0.005; } // Positioned lower

		int getGameSettingsX() { return scrWidth * 0.01; } // 1% from the left edge
		int getGameSettingsY() { return getGameHomeY(); } // Align vertically with Home button

	    int getQuitX() { return (scrWidth - getButtonWidth()) / 2; } // Centered
	    int getQuitY() { return scrHeight - getButtonHeight() - (scrHeight * 0.02); } // teensy bit above bottom edge
	}

	void resizeButtons() {
		buttonWidth = mform::getButtonWidth();
		buttonHeight = mform::getButtonHeight();
		titleWidth = mform::getTitleWidth();
		titleHeight = mform::getTitleHeight();
	}

	void resizeGameButtons() {
		gameButtonWidth = mform::getGameButtonWidth();
		gameButtonHeight = mform::getGameButtonHeight();
	}

	void resetHomeRects() {
		titleRect = {mform::getTitleX(), mform::getTitleY(), titleWidth, titleHeight};
	    playRect = {mform::getPlayX(), mform::getPlayY(), buttonWidth, buttonHeight};
	    returnRect = {mform::getReturnX(), mform::getReturnY(), buttonWidth, buttonHeight};
	    settingsRect = {mform::getSettingsX(), mform::getSettingsY(), buttonWidth, buttonHeight};
	}
	void resetSettingsRects() {
		titleRect = {mform::getTitleX(), mform::getTitleY(), titleWidth, titleHeight};
	    returnRect = {mform::getReturnX(), mform::getReturnY(), buttonWidth, buttonHeight};
	    playRect = {mform::getPlayX(), mform::getPlayY(), buttonWidth, buttonHeight}; // Using default playX, playY
	    homeRect = {mform::getHomeX(), mform::getHomeY(), buttonWidth, buttonHeight}; // Using default homeX, homeY (ahh)
	}
	void resetGameRects() {
		backgroundRect = {0, 0, scrWidth, scrHeight};
	    quitRect = {mform::getQuitX(), mform::getQuitY(), gameButtonWidth, gameButtonHeight};
	    gHomeRect = {mform::getGameHomeX(), mform::getGameHomeY(), gameButtonWidth, gameButtonHeight};
	    gSettingsRect = {mform::getGameSettingsX(), mform::getGameSettingsY(), gameButtonWidth, gameButtonHeight};
	}

	void resetHomeButtons() {
	    resizeButtons();
	    resetHomeRects();
	}

	void resetSettingsButtons() {
	    resizeButtons();
	    resetSettingsRects();
	}

	void resetGameButtons() {
	    resizeGameButtons();
	    resetGameRects();
	}

	SDL_Texture* cachedTexture = nullptr;
	void loadCachedTexture() { // loads and draws the cachedTexture, but nothing more
		if (!gDeck) {
			std::cerr<<"Deck object null when creating cachedTexture"<<std::endl;
			return;
		}
		if (!cachedTexture) {
			cachedTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, scrWidth, scrHeight);
			if (!cachedTexture) {
				std::cerr<<"Unable to create cached texture"<<std::endl; return;
			}
			SDLW::setTarget(cachedTexture);
	    	SDLW::renderClear();

	    	SDLW::drawToTexture(backgroundTexture, backgroundRect);

	    	gDeck->drawAllPiles(); // a hack, but one that's important
	    	// does not draw buttons when dragging
		}
	}
	void destroyCachedTexture() {
		if (cachedTexture) {
			SDL_DestroyTexture(cachedTexture);
			cachedTexture = nullptr;
		}
	}
	void drawBackground() {
		SDLW::drawToWindow(backgroundTexture, backgroundRect);
	}
	void drawGameButtons() {
		SDLW::drawToWindow(homeTexture, gHomeRect);
		SDLW::drawToWindow(settingsTexture, gSettingsRect);
		SDLW::drawToWindow(quitTexture, quitRect);
	}
	void drawEverything() {
		drawBackground();
		if (!gDeck) {
			std::cerr<<"Deck invalid for drawEverything()"<<std::endl;
		} else {
			gDeck->drawAllPiles();
		}
		drawGameButtons();
	}
}

void resetRenderLogicSize() {
	if (gRenderer) {
		SDL_RenderSetLogicalSize(gRenderer, scrWidth, scrHeight);
	} else {
		std::cerr<<"gRenderer is null in resetRenderLogicSize()"<<std::endl;
	}
}

// ---- GAME LOOP HERE ----

void resetGameSizes() { // handled by resizeHandler
	Meta::resetGameButtons();
    gDeck->onResize();
}
void resizeHandler(int new_width, int new_height) {
    scrWidth = new_width;
    scrHeight = new_height;
    resetRenderLogicSize();

    std::cout<<"New scrWidth: "<<scrWidth<<" and scrHeight "<<scrHeight<<std::endl;

    // Reset button sizes and positions based on the current screen
    switch (screen) {
        case Screen::Home:
            Meta::resetHomeButtons();
            if (game_is_running) resetGameSizes();
            break;
        case Screen::Settings:
            Meta::resetSettingsButtons();
            if (game_is_running) resetGameSizes();
            break;
        case Screen::Game:
            resetGameSizes();
            break;
    }

    has_changed = true;
}

enum class GamePerspective {
	Home,
	Settings,
	Quit,
	Nothing
};
GamePerspective gPersp = GamePerspective::Nothing;

void quitGame() {
	if (Meta::cachedTexture) Meta::destroyCachedTexture();
	if (gDeck) delete gDeck;
	if (dragged) Operations::clearDragged();
	gDeck = nullptr;
	game_is_running = false;
	screen = Screen::Home;
	has_changed = true;
	clearChangeListener();
	Operations::clearFoundationRegistry();
	Meta::resetHomeButtons();
}

void GameLoop() {
    SDL_Event e;

    if (!game_is_running) {
    	gDeck = new Deck(DEFAULT_SUIT_LENGTH, DEFAULT_NO_OF_SUITS, DEFAULT_NO_OF_TABLEAUS);
    	if (gDeck) {
    		game_is_running = true;
    		gDeck->renderAllPiles();
    		std::cout<<"Game started successfully or whatever"<<std::endl;
    	} else {
    		std::cerr<<"Could not start a new game"<<std::endl;
    		gStatus = GameStatus::DeckInitError;
    	}
    }

    if (dragged && has_changed) {
    	SDLW::setWindowTarget();
    	SDLW::renderClear();

    	if (!Meta::cachedTexture) {
    		std::cerr<<"cachedTexture does not exist when drawing in GameLoop"<<std::endl;
    	} else {
    		SDLW::drawToWindow(Meta::cachedTexture, {0, 0, scrWidth, scrHeight});
    		std::cout<<"Window has been drawn while dragging"<<std::endl;
    	}
    	if (!gStack) {
    		std::cerr<<"gStack is null when drawing in GameLoop"<<std::endl;
    	} else {
    		SDLW::drawToWindow(gStack->getStackTexture(), gStack->getStackRect());
    		const SDL_Rect& rect = gStack->getStackRect();
    		std::cout<<"Stack has been drawn while dragging with dimensions "<<rect.x<<","<<rect.y<<","<<rect.w<<","<<rect.h<<std::endl;
    	}

    	SDLW::renderPresent();

    	has_changed = false;

    } else if (has_changed && !changeListener.empty()) {

    	if (!gDeck) {
    		std::cerr<<"gDeck null for when changeListener is filled"<<std::endl;
    		return;
    	}

    	for (int i=0; i<changeListener.size(); i++) {
    		switch (changeListener[i]) {
    		case ChangeListener::Stock:
    			gDeck->renderPileAgain(gDeck->getStock());
    			// change_idx[0] = change_idx[1]; // extra hacky
    			break;
    		case ChangeListener::Waste:
    			gDeck->renderPileAgain(gDeck->getWaste());
    			// change_idx[0] = change_idx[1]; // extra hacky
    			break;
    		case ChangeListener::Tableau:
    			// gDeck->renderPileAgain(gDeck->getTableau(change_idx[i]));
    			// change_idx[0] = change_idx[1]; // hacky
    			gDeck->renderPileAgain(gDeck->getTableau(changeIDXs[i])); // hacky? not anymore 😈
    			break;
    		case ChangeListener::Foundation:
    			// gDeck->renderPileAgain(gDeck->getFoundation(change_idx[0]));
    			// change_idx[0] = change_idx[1];
    			gDeck->renderPileAgain(gDeck->getFoundation(changeIDXs[i]));
    			break;
    		}
    	}
    	// changeListener.clear(); change_idx[0] = 0; change_idx[1] = 0; // all change_idx's are hacky
    	clearChangeListener();

    	SDLW::setWindowTarget();
    	SDLW::renderClear();

    	Meta::drawBackground();

    	gDeck->drawAllPiles();

    	Meta::drawGameButtons();

    	SDLW::renderPresent();

    	std::cout<<"changeListener handled"<<std::endl;

    	has_changed = false;

    } else if (has_changed && changeListener.empty()) {
    	SDLW::setWindowTarget();
    	SDLW::renderClear();
    	Meta::drawEverything();

    	std::cout<<"does it get to init render and draw in gameLoop?"<<std::endl;
    	SDLW::renderPresent();

    	has_changed = false;
    }

    while (SDL_PollEvent(&e)!=0) {
    	// to handle EVERYTHINg
    	if (e.type == SDL_QUIT) {
    		if (gDeck) delete gDeck;
    		gDeck = nullptr;
    		quit = true;
    	} else if (e.type == SDL_WINDOWEVENT) {
    		if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
    			resizeHandler(e.window.data1, e.window.data2);
    		}
    	} else if (e.type == SDL_MOUSEBUTTONDOWN) {
    		/* if (Operations::mouseDownHandled(e, *gDeck)) {
    			// game operations closure here for mousedown
    		} else {
    			SDL_Point mp = {e.button.x, e.button.y};
    			if (!dragged) {
    				if (SDLW::mouseInRect(Meta::gHomeRect, mp)) {
    					gPersp = GamePerspective::Home;
    				} else if (SDLW::mouseInRect(Meta::quitRect, mp)) {
    					gPersp = GamePerspective::Quit;
    				} else if (SDLW::mouseInRect(Meta::gSettingsRect, mp)) {
    					gPersp = GamePerspective::Settings;
    				}
    			}
    		} */
    		// new version - checks buttons first
    		if (!dragged) {
    			SDL_Point mp = {e.button.x, e.button.y};
    			if (SDLW::mouseInRect(Meta::gHomeRect, mp)) {
					gPersp = GamePerspective::Home;
				} else if (SDLW::mouseInRect(Meta::quitRect, mp)) {
					gPersp = GamePerspective::Quit;
				} else if (SDLW::mouseInRect(Meta::gSettingsRect, mp)) {
					gPersp = GamePerspective::Settings;
				} else {
					Operations::mouseDownHandled(e, *gDeck);
				}
    		} else {
    			Operations::mouseDownHandled(e, *gDeck);
    		}

    	} else if (e.type == SDL_MOUSEBUTTONUP) {
    		if (Operations::mouseUpHandled(e, *gDeck)) {
    			// game operations closure for mouseup
    		} else if (!dragged && gPersp != GamePerspective::Nothing) {
    			if (gPersp == GamePerspective::Home) {
    				screen = Screen::Home;
    				Meta::resetHomeButtons();
    			} else if (gPersp == GamePerspective::Settings) {
    				screen = Screen::Settings;
    				Meta::resetSettingsButtons();
    			} else if (gPersp == GamePerspective::Quit) {
    				// deck deletion here and subsequent return to home
    				quitGame();
    			}
    			gPersp = GamePerspective::Nothing;
    			has_changed = true;
    		}
    	} else if (dragged && e.type == SDL_MOUSEMOTION) {
    		Operations::mouseMotionHandled(e, *gDeck);
    	}
    }
}

enum class SettingsPerspective {
	Play,
	Home,
	Nothing
};
SettingsPerspective settPersp = SettingsPerspective::Nothing;
void SettingsLoop() {
    // Settings loop logic here
    SDL_Event e;

    if (has_changed) {
    	SDLW::setWindowTarget();
    	SDLW::renderClear();

    	SDLW::drawToWindow(Meta::titleTexture, Meta::titleRect);

    	// for play or return
    	if (game_is_running) {
    		SDLW::drawToWindow(Meta::returnTexture, Meta::returnRect);
    	} else {
    		SDLW::drawToWindow(Meta::playTexture, Meta::playRect);
    	}

    	// for home
    	SDLW::drawToWindow(Meta::homeTexture, Meta::homeRect);

    	SDLW::renderPresent();
    	has_changed = false;
    }

    while (SDL_PollEvent(&e)!=0) {
    	if (e.type == SDL_QUIT) {
    		if (game_is_running && gDeck) {
	   			delete gDeck;
	   			gDeck = nullptr;
	    	}
    		quit = true;
    	}
    	else if (e.type == SDL_WINDOWEVENT) {
    		if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
    			resizeHandler(e.window.data1, e.window.data2);
    		}
    	}
    	else if (e.type == SDL_MOUSEBUTTONDOWN) {
    		SDL_Point mouse_point = {e.button.x, e.button.y};
    		if (SDLW::mouseInRect(Meta::playRect, mouse_point)) {
    			settPersp = SettingsPerspective::Play;
    		} else if (SDLW::mouseInRect(Meta::homeRect, mouse_point)) {
    			settPersp = SettingsPerspective::Home;
    		}
    	} else if (e.type == SDL_MOUSEBUTTONUP) {
    		if (settPersp != SettingsPerspective::Nothing) {
    			if (settPersp == SettingsPerspective::Play) {
    				screen = Screen::Game;
    				Meta::resetGameButtons();
    			} else if (settPersp == SettingsPerspective::Home) {
    				screen = Screen::Home;
    				Meta::resetHomeButtons();
    			}
    			has_changed = true;
    			settPersp = SettingsPerspective::Nothing;
    		}
    	}
    }
}

enum class HomePerspective {
	Play,
	Settings,
	Nothing
};
HomePerspective hPerp = HomePerspective::Nothing;
void HomeLoop() {
    // Home loop logic here
    SDL_Event e;

    if (has_changed) {
    	// for title
    	SDLW::setWindowTarget();
    	SDLW::renderClear();

    	SDLW::drawToWindow(Meta::titleTexture, Meta::titleRect);

    	// for play OR return/resume
    	// SDL_Texture* go_to_game_texture;
    	// SDL_Rect go_to_game_rect;
    	if (game_is_running) {
    		// go_to_game_texture = Meta::returnTexture;
    		// go_to_game_rect = Meta::returnRect;
    		SDLW::drawToWindow(Meta::returnTexture, Meta::returnRect);
    	} else {
    		// go_to_game_texture = Meta::playTexture;
    		// go_to_game_rect = Meta::playRect;
    		SDLW::drawToWindow(Meta::playTexture, Meta::playRect);
    	}
    	// SDLW::drawToWindow(go_to_game_texture, go_to_game_rect);

    	// for settings
    	SDLW::drawToWindow(Meta::settingsTexture, Meta::settingsRect);

    	SDLW::renderPresent();
    	has_changed = false;
    }

    while (SDL_PollEvent(&e)!=0) {
    	if (e.type == SDL_QUIT) {
    		if (game_is_running && gDeck) {
	   			delete gDeck;
	   			gDeck = nullptr;
	    	}
    		quit = true;
    	}
    	else if (e.type == SDL_WINDOWEVENT) {
    		if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
    			resizeHandler(e.window.data1, e.window.data2);
    		}
    	}
    	else if (e.type == SDL_MOUSEBUTTONDOWN) {
    		SDL_Point mouse_point = {e.button.x, e.button.y};
    		if (SDLW::mouseInRect(Meta::playRect, mouse_point)) {
    			hPerp = HomePerspective::Play;
    		} else if (SDLW::mouseInRect(Meta::settingsRect, mouse_point)) {
    			hPerp = HomePerspective::Settings;
    		}
    	} else if (e.type == SDL_MOUSEBUTTONUP) {
    		if (hPerp != HomePerspective::Nothing) {
    			if (hPerp == HomePerspective::Play) {
    				screen = Screen::Game;
    				Meta::resetGameButtons();
    			} else if (hPerp == HomePerspective::Settings) {
    				screen = Screen::Settings;
    				Meta::resetSettingsButtons();
    			}
    			has_changed = true;
    			hPerp = HomePerspective::Nothing;
    		}
    	}
    }
}

enum class InitStatus {
    Success,
    ErrorInitSDL,
    ErrorInitIMG,
    ErrorCreatingWindow,
    ErrorCreatingRenderer,
    ErrorLoadingMetaTextures
};

InitStatus init() {
    // Initialization logic here
    if (SDL_Init(SDL_INIT_VIDEO)<0) {
    	std::cerr<<"Can't initialise SDL:"<<SDL_GetError()<<std::endl;
    	return InitStatus::ErrorInitSDL;
    }
    if (IMG_Init(IMG_INIT_PNG)==0) {
    	std::cerr<<"Can't initialise SDL Image: "<<SDL_GetError()<<std::endl;
    	return InitStatus::ErrorInitIMG;
    }
    gWindow = SDL_CreateWindow("asolGUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, scrWidth, scrHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!gWindow) {
    	std::cerr<<"Could not create SDL Window: "<<SDL_GetError()<<std::endl;
    	return InitStatus::ErrorCreatingWindow;
    }
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!gRenderer) {
    	std::cerr<<"Could not create game renderer: "<<SDL_GetError()<<std::endl;
    	return InitStatus::ErrorCreatingRenderer;
    }
    resetRenderLogicSize();

    if (!Meta::initTextures()) {
    	std::cerr<<"Could not load the meta textures"<<std::endl;
    	return InitStatus::ErrorLoadingMetaTextures;
    }

    // for error logging speedup
    std::ios_base::sync_with_stdio(false);

    return InitStatus::Success;
}

void close() {
	Meta::closeTextures();
	if (gRenderer) {
		SDL_DestroyRenderer(gRenderer);
		gRenderer = nullptr;
	}
	if (gWindow) {
		SDL_DestroyWindow(gWindow);
		gWindow = nullptr;
	}
	IMG_Quit();
	SDL_Quit();
	std::cout<<"Close finished successfully"<<std::endl;
}

int main(int argc, char* argv[]) {
    InitStatus initStatus = init();

    if (initStatus == InitStatus::Success) {

        Meta::resizeButtons();
        Meta::resetHomeButtons();

        while (!quit) {
            switch (screen) {
                case Screen::Game:
                    GameLoop();
                    SDL_Delay(1);
                    break;
                case Screen::Settings:
                    SettingsLoop();
                    SDL_Delay(1);
                    break;
                default:
                    HomeLoop();
                    SDL_Delay(1);
                    break;
            }
        }

        if (game_is_running) delete gDeck;

        // Close resources here (sorry)
        close();
    } else {

    	if (initStatus == InitStatus::ErrorCreatingRenderer) {
    		if (gWindow) SDL_DestroyWindow(gWindow);
    		gWindow = nullptr;
    	}
        std::cerr << "Initialization failure in init" << std::endl;
        std::cin.get();
    }

    return 0;
}
// welcome back int main() 😔