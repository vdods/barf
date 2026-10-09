// 2026.10.02 - Copyright Daniel Palm - Licensed under Apache 2.0

#![forbid(unsafe_code)]
include!("parser.rs");

fn integer(value: i64) -> Token { Token::new(Terminal::INTEGER as u32, value) }
fn punctuation(value: u8) -> Token { Token::without_data(u32::from(value)) }

fn main() {
    let tokens = vec![integer(2), punctuation(b'+'), integer(3), punctuation(b'*'), integer(4), Token::without_data(Terminal::END_ as u32)];
    assert_eq!(Parser::new().parse(tokens, Nonterminal::input).expect("valid expression"), 14);

    let tokens = vec![punctuation(b'('), integer(2), punctuation(b'+'), integer(3), punctuation(b')'), punctuation(b'*'), integer(4), Token::without_data(Terminal::END_ as u32)];
    assert_eq!(Parser::new().parse(tokens, Nonterminal::input).expect("valid expression"), 20);
}
