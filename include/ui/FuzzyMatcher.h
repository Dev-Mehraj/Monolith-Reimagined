// FuzzyMatcher.h - Fuzzy Matching Utilities
#ifndef MONOLITH_UI_FUZZYMATCHER_H
#define MONOLITH_UI_FUZZYMATCHER_H

#include <QString>
#include <QVector>

namespace monolith {

/**
 * @brief High-performance fuzzy string matcher using Smith-Waterman algorithm
 * 
 * Provides VS Code-style fuzzy matching for:
 * - Command palette filtering
 * - Quick open file search
 * - Symbol search
 */
class FuzzyMatcher
{
public:
    /**
     * @brief Calculate fuzzy match score between pattern and text
     * 
     * Uses a combination of:
     * - Character-by-character matching with gaps allowed
     * - Bonus points for consecutive matches
     * - Bonus points for word boundary matches (camelCase, snake_case, path separators)
     * - Penalty for gaps and non-matching characters
     * 
     * @param pattern The search pattern (e.g., "mwl" matches "MainWindowLayout")
     * @param text The text to match against
     * @return Score >= 0 if match found, 0 if no match. Higher is better.
     */
    static int match(const QString& pattern, const QString& text);
    
    /**
     * @brief Get the positions of matched characters for highlighting
     * 
     * @param pattern The search pattern
     * @param text The text to match against
     * @return Vector of character indices in text that matched
     */
    static QVector<int> matchPositions(const QString& pattern, const QString& text);
    
    /**
     * @brief Quick check if pattern matches text (without score calculation)
     * 
     * @param pattern The search pattern
     * @param text The text to match against
     * @return true if all pattern characters can be found in order in text
     */
    static bool matches(const QString& pattern, const QString& text);
    
    /**
     * @brief Check if pattern should prefer prefix matching
     * 
     * @param pattern The search pattern
     * @return true if pattern looks like it wants prefix matching
     */
    static bool isPrefixPattern(const QString& pattern);
    
private:
    /**
     * @brief Smith-Waterman local alignment algorithm implementation
     * 
     * @param pattern Search pattern
     * @param text Text to search in
     * @param positions Output parameter for match positions
     * @return Match score
     */
    static int smithWaterman(const QString& pattern, const QString& text, QVector<int>& positions);
    
    /**
     * @brief Calculate bonus for a character match based on context
     * 
     * Bonuses are given for:
     * - Exact case match
     * - Word boundaries (after _, -, /, space, or camelCase transition)
     * - Consecutive matches
     * - Beginning of text
     * 
     * @param patternChar Character from pattern
     * @param textChar Character from text
     * @param posInText Position in text string
     * @param textLen Total length of text
     * @param prevMatched Whether previous character was matched
     * @return Bonus points to add
     */
    static int calculateBonus(char patternChar, char textChar, int posInText, int textLen, bool prevMatched);
    
    /**
     * @brief Check if character is at a word boundary
     */
    static bool isWordBoundary(const QString& text, int pos);
    
    /**
     * @brief Check if character is uppercase
     */
    static bool isUpperCase(QChar c);
    
    /**
     * @brief Simple case-insensitive character comparison
     */
    static bool charsMatch(QChar patternChar, QChar textChar);
};

} // namespace monolith

#endif // MONOLITH_UI_FUZZYMATCHER_H
