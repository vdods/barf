// 2026.09.30 - Copyright Daniel Palm - Licensed under Apache 2.0

#![forbid(unsafe_code)]

include!("scanner.rs");

fn main() {
    let mut scanner = Scanner::new(b"sum + 42".to_vec());
    assert_eq!(scanner.scan(), Token::Word("sum".to_owned()));
    assert_eq!(scanner.scan(), Token::Plus);
    assert_eq!(scanner.scan(), Token::Integer(42));
    assert_eq!(scanner.scan(), Token::End);

    scanner.replace_input(b"~HeLLo".to_vec());
    assert_eq!(scanner.scan(), Token::CaseInsensitive);
    assert_eq!(scanner.scan(), Token::End);

    scanner.replace_input(b"ab \"hello world\"".to_vec());
    assert_eq!(scanner.scan(), Token::A);
    assert_eq!(scanner.scan(), Token::Word("b".to_owned()));
    assert_eq!(scanner.scan(), Token::Quoted("\"hello world\"".to_owned()));
    assert_eq!(scanner.current_state_machine(), StateMachine::MAIN);
    assert_eq!(scanner.scan(), Token::End);

    scanner.replace_input(b"@".to_vec());
    assert_eq!(scanner.scan(), Token::Bad(b'@'));

    scanner.replace_input(b"stream".to_vec());
    scanner.append(b"ed");
    assert_eq!(scanner.scan(), Token::Word("streamed".to_owned()));
    assert_eq!(scanner.scan(), Token::End);
}
